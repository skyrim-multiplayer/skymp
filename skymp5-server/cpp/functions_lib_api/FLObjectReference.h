#pragma once
#include "PartOne.h"
#include <JsEngine.h>

void RegisterObjectReferenceApi(std::shared_ptr<PartOne> partOne);

JsValue ObjectReferenceCtor(std::shared_ptr<PartOne> partOne,
                            const JsFunctionArguments& args);

JsValue Activate(std::shared_ptr<PartOne> partOne,
                 const JsFunctionArguments& args);

JsValue AddItem(std::shared_ptr<PartOne> partOne,
                const JsFunctionArguments& args);

JsValue Disable(std::shared_ptr<PartOne> partOne,
                const JsFunctionArguments& args);

JsValue DoReloot(std::shared_ptr<PartOne> partOne,
                 const JsFunctionArguments& args);

JsValue Enable(std::shared_ptr<PartOne> partOne,
               const JsFunctionArguments& args);

JsValue GetAngle(std::shared_ptr<PartOne> partOne,
                 const JsFunctionArguments& args);

JsValue GetBaseId(std::shared_ptr<PartOne> partOne,
                  const JsFunctionArguments& args);

JsValue GetCellOrWorld(std::shared_ptr<PartOne> partOne,
                       const JsFunctionArguments& args);

JsValue GetAnimationVariableBool(std::shared_ptr<PartOne> partOne,
                                 const JsFunctionArguments& args);

JsValue GetInventory(std::shared_ptr<PartOne> partOne,
                     const JsFunctionArguments& args);

JsValue GetPos(std::shared_ptr<PartOne> partOne,
               const JsFunctionArguments& args);

JsValue IsActivationBlocked(std::shared_ptr<PartOne> partOne,
                            const JsFunctionArguments& args);

JsValue IsDisabled(std::shared_ptr<PartOne> partOne,
                   const JsFunctionArguments& args);

JsValue IsHarvested(std::shared_ptr<PartOne> partOne,
                    const JsFunctionArguments& args);

JsValue IsOpen(std::shared_ptr<PartOne> partOne,
               const JsFunctionArguments& args);

JsValue RemoveAllItems(std::shared_ptr<PartOne> partOne,
                       const JsFunctionArguments& args);

JsValue RemoveItem(std::shared_ptr<PartOne> partOne,
                   const JsFunctionArguments& args);

JsValue SetActivationBlocked(std::shared_ptr<PartOne> partOne,
                             const JsFunctionArguments& args);

JsValue SetAngle(std::shared_ptr<PartOne> partOne,
                 const JsFunctionArguments& args);

JsValue SetAnimationVariableBool(std::shared_ptr<PartOne> partOne,
                                 const JsFunctionArguments& args);

JsValue SetCellOrWorld(std::shared_ptr<PartOne> partOne,
                       const JsFunctionArguments& args);

JsValue SetHarvested(std::shared_ptr<PartOne> partOne,
                     const JsFunctionArguments& args);

JsValue SetPos(std::shared_ptr<PartOne> partOne,
               const JsFunctionArguments& args);

JsValue MoveTo(std::shared_ptr<PartOne> partOne,
               const JsFunctionArguments& args);

JsValue PlaceAtMe(std::shared_ptr<PartOne> partOne,
                  const JsFunctionArguments& args);

JsValue GetDistance(std::shared_ptr<PartOne> partOne,
                    const JsFunctionArguments& args);

JsValue GetLinkedDoorId(std::shared_ptr<PartOne> partOne,
                        const JsFunctionArguments& args);
