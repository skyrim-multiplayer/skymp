#pragma once

#include <concepts>
#include <cstdint>
#include <exception>
#include <fmt/format.h>
#include <limits>
#include <napi.h>
#include <optional>
#include <simdjson.h>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include "concepts/Concepts.h"

class NapiOutputArchive
{
public:
    explicit NapiOutputArchive(Napi::Env env) : m_env(env) {}

    template <IntegralConstant T>
    NapiOutputArchive& Serialize(const char* key, T& input)
    {
        OutputAsObject().Set(key, Napi::Number::New(m_env, static_cast<int64_t>(input)));
        return *this;
    }

    template <StringLike T>
    NapiOutputArchive& Serialize(const T& input)
    {
        static_assert(!sizeof(T), "not string");
        return *this;
    }

    NapiOutputArchive& Serialize(const std::string& input)
    {
        OutputAsOther() = Napi::String::New(m_env, input);
        return *this;
    }

    template <typename T, std::size_t N>
    NapiOutputArchive& Serialize(const std::array<T, N>& input)
    {
        auto array = Napi::Array::New(m_env, N);
        for (size_t i = 0; i < N; ++i)
        {
            NapiOutputArchive elementAr(m_env);
            elementAr.Serialize(input[i]);
            array.Set(i, elementAr.extract_output());
        }
        OutputAsOther() = array;
        return *this;
    }

    template <ContainerLike T>
    NapiOutputArchive& Serialize(const T& input)
    {
        auto array = Napi::Array::New(m_env, input.size());
        size_t index = 0;
        for (const auto& element : input)
        {
            NapiOutputArchive elementAr(m_env);
            elementAr.Serialize(element);
            array.Set(index++, elementAr.extract_output());
        }
        OutputAsOther() = array;
        return *this;
    }

    template <Arithmetic T>
    NapiOutputArchive& Serialize(const T& input)
    {
        OutputAsOther() = Napi::Number::New(m_env, static_cast<double>(input));
        return *this;
    }

    template <NoneOfTheAbove T>
    NapiOutputArchive& Serialize(const T& input)
    {
        input.Serialize(*this);
        return *this;
    }

    template <class T>
    NapiOutputArchive& Serialize(const char* key, const std::optional<T>& input)
    {
        if (input.has_value())
        {
            NapiOutputArchive ar(m_env);
            ar.Serialize(input.value());
            OutputAsObject().Set(key, ar.extract_output());
        }
        return *this;
    }

    template <class T>
    NapiOutputArchive& Serialize(const char* key, const T& input)
    {
        NapiOutputArchive ar(m_env);
        ar.Serialize(input);
        OutputAsObject().Set(key, ar.extract_output());
        return *this;
    }

    Napi::Value extract_output()
    {
        if (m_outputOther)
        {
            return std::move(*m_outputOther);
        }

        if (m_outputObject)
        {
            return std::move(*m_outputObject);
        }

        throw std::runtime_error("Uninitialized field!");
    }

private:
    Napi::Object& OutputAsObject()
    {
        if (m_outputOther)
        {
            throw std::runtime_error("Not object!");
        }

        if (!m_outputObject)
        {
            m_outputObject = Napi::Object::New(m_env);
        }

        return *m_outputObject;
    }

    Napi::Value& OutputAsOther()
    {
        if (m_outputObject)
        {
            throw std::runtime_error("Not other!");
        }

        if (!m_outputOther)
        {
            m_outputOther.emplace(m_env.Undefined());
        }

        return *m_outputOther;
    }

    Napi::Env m_env;
    std::optional<Napi::Value> m_outputOther;
    std::optional<Napi::Object> m_outputObject;
};