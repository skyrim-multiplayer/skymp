#pragma once
#include "PartOne.h"
#include <JsEngine.h>

void RegisterActorApi(std::shared_ptr<PartOne> partOne);

JsValue ActorCtor(std::shared_ptr<PartOne> partOne,
                  const JsFunctionArguments& args);

JsValue DamageActorValue(std::shared_ptr<PartOne> partOne,
                         const JsFunctionArguments& args);

JsValue GetAppearance(std::shared_ptr<PartOne> partOne,
                      const JsFunctionArguments& args);

JsValue GetBaseActorValues(std::shared_ptr<PartOne> partOne,
                           const JsFunctionArguments& args);

JsValue GetBounds(std::shared_ptr<PartOne> partOne,
                  const JsFunctionArguments& args);

JsValue GetEquipment(std::shared_ptr<PartOne> partOne,
                     const JsFunctionArguments& args);

JsValue GetRaceId(std::shared_ptr<PartOne> partOne,
                  const JsFunctionArguments& args);

JsValue Kill(std::shared_ptr<PartOne> partOne,
             const JsFunctionArguments& args);

JsValue Respawn(std::shared_ptr<PartOne> partOne,
                const JsFunctionArguments& args);

JsValue RestoreActorValue(std::shared_ptr<PartOne> partOne,
                          const JsFunctionArguments& args);

JsValue SetActorValuesPercentages(std::shared_ptr<PartOne> partOne,
                                  const JsFunctionArguments& args);

JsValue SetRaceMenuOpen(std::shared_ptr<PartOne> partOne,
                        const JsFunctionArguments& args)
