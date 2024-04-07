#pragma once

namespace espm {

using RecordFlagsType = std::uint32_t;

/// <summary>
/// A ulong used to identify a data object. May refer to a data object from a
/// mod or new object created in-game.
/// </summary>
using formId = std::uint32_t;

/// <summary>
/// 'Localized string', a ulong that is used as an index to look up string data
/// information in one of the string tables. Note that the record header for
/// the TES4 record indicates if the file is localized or not. If not, all
/// lstrings are zstrings.
/// </summary>
using lstring = std::uint32_t;

/// <summary>
/// Zero terminated string
/// </summary>
using zstring = std::string;

}
