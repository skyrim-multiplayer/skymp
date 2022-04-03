Scriptname SweetPie Hidden
Int Function GetBuyPieStartMessage(Int licenseIndex) native global
Int Function GetBuyPieFailMessage(Int licenseIndex) native global
Int Function GetBuyPieFinishMessage(Int licenseIndex) native global
Int[] Function GetBuyPieRequiredItems(Int licenseIndex) native global
Int Function GetBuyPieRequiredItemCount(Int licenseIndex, Int requiredItemIndex) native global
Int Function GetBuyPieReturnItem(Int licenseIndex, Int requiredItemIndex, Int returnItemIndex) native global
Int Function GetBuyPieReturnItemIndex(Int licenseIndex, Int requiredItemIndex) native global
Int Function GetBuyPieReturnItemCount(Int licenseIndex, Int requiredItemIndex, Int returnItemIndex) native global
Int Function GetBuyPieCommissionItem(Int licenseIndex) native global
Int Function GetBuyPieCommissionSize(Int licenseIndex) native global
Int[] Function GetBuyPieLicenses() native global
Function SPLog(ObjectReference selfRef, ObjectReference Ref, string TextToPrint) native global
Function SPDumpActorArray(ObjectReference Ref, string TextToPrint, Actor[] Actors) native global
