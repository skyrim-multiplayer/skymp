Scriptname SweetBuyPieDialog extends ObjectReference

Event OnActivate(ObjectReference akActionRef)
	Actor Client = akActionRef as Actor
	ObjectReference Broker = self
	if(Client && Broker)
		Int[] Licenses = SweetPie.GetBuyPieLicenses()
		Int nLIndex = -1
		Int nIndex = 0
		while(nIndex < Licenses.Length)
			if(Broker.GetItemCount(Game.GetForm(Licenses[nIndex])))
				nLIndex = nIndex
			endif
			nIndex +=1
		endwhile
		nIndex = -1
		if(nLIndex >= 0)
			Message mStart = Game.GetForm(SweetPie.GetBuyPieStartMessage(nLIndex)) as Message
			Int[] Required = SweetPie.GetBuyPieRequiredItems(nLIndex)
			nIndex = mStart.Show()
			if(nIndex >=0 && nIndex < Required.Length)
				Form RequiredItem = Game.GetForm(Required[nIndex])
				Int RequiredItemCount = SweetPie.GetBuyPieRequiredItemCount(nLIndex, nIndex)
				if(Client.GetItemCount(RequiredItem) >= RequiredItemCount)
					Message mFinish = Game.GetForm(SweetPie.GetBuyPieFinishMessage(nLIndex)) as Message
					Int RewardIndex = SweetPie.GetBuyPieReturnItemIndex(nLIndex, nIndex)
					mFinish.Show()
					Client.RemoveItem(RequiredItem, RequiredItemCount)
					Client.AddItem(Game.GetForm(SweetPie.GetBuyPieReturnItem(nLIndex, nIndex, RewardIndex)), SweetPie.GetBuyPieReturnItemCount(nLIndex, nIndex, RewardIndex))
					Broker.AddItem(Game.GetForm(SweetPie.GetBuyPieCommissionItem(nLIndex)), SweetPie.GetBuyPieCommissionSize(nLIndex))
				else
					Message mFail = Game.GetForm(SweetPie.GetBuyPieFailMessage(nLIndex)) as Message
					mFail.Show()
				endif
			endif
		endif
	endif
EndEvent
