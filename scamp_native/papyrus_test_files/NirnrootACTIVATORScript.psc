Scriptname NirnrootACTIVATORScript extends ObjectReference Hidden 
{Script to only be applied to the NirnrootACTIVATOR base object.}

Ingredient Property Nirnroot Auto
Message Property HarvestNirnrootMESSAGE Auto
Activator Property NirnrootGlowACTIVATOR Auto
Activator Property NirnrootEmptyACTIVATOR Auto

ObjectReference Property NirnrootGlowReference Auto Hidden
ObjectReference Property NirnrootEmptyReference Auto Hidden
ObjectReference Property EmptyRef Auto Hidden
Bool Property HasBeenLoaded Auto Hidden

Float NirnScale


EVENT OnCellAttach()

	if (self.IsDisabled() == FALSE)
		NirnrootGlowReference = PlaceAtMe(NirnRootGlowACTIVATOR)
		NirnrootEmptyReference = PlaceAtMe(NirnRootEmptyACTIVATOR)
		NirnrootEmptyReference.DisableNoWait()
	endif

endEVENT



EVENT OnReset()

	if (self.IsDisabled() == TRUE)
		self.Enable()
		GoToState("WaitingForHarvest")
	endif
	
endEVENT



Auto STATE WaitingForHarvest

	EVENT onActivate(ObjectReference TriggerRef)
		GoToState("AlreadyHarvested")
		Nirnscale = Self.GetScale()
		NirnrootEmptyReference.SetScale(Nirnscale)
		NirnrootEmptyReference.EnableNoWait()
		(TriggerRef as Actor).additem(Nirnroot, 1)
		Game.IncrementStat( "Nirnroots Found" )
		NirnRootGlowReference.DisableNoWait()
		NirnRootGlowReference.Delete()

		self.DisableNoWait()
		
	endEVENT

endSTATE



STATE AlreadyHarvested
	

endSTATE
