Scriptname PlayerBookShelfContainerScript extends ObjectReference
{This script has been overhauled to fix several issues listed in USLEEP 3.0.1 - Bug #19515. This overhaul was contributed by Adria.}

import debug
import utility

Form Property LItemBookClutter Auto
{Clutter item list to fill the shelf with}

Keyword Property BookShelfBook01 Auto
Keyword Property BookShelfBook02 Auto
Keyword Property BookShelfBook03 Auto
Keyword Property BookShelfBook04 Auto
Keyword Property BookShelfBook05 Auto
Keyword Property BookShelfBook06 Auto
Keyword Property BookShelfBook07 Auto
Keyword Property BookShelfBook08 Auto
Keyword Property BookShelfBook09 Auto
Keyword Property BookShelfBook10 Auto
Keyword Property BookShelfBook11 Auto
Keyword Property BookShelfBook12 Auto
Keyword Property BookShelfBook13 Auto
Keyword Property BookShelfBook14 Auto
Keyword Property BookShelfBook15 Auto
Keyword Property BookShelfBook16 Auto
Keyword Property BookShelfBook17 Auto
Keyword Property BookShelfBook18 Auto
Keyword Property BookShelfTrigger01 Auto
Keyword Property BookShelfTrigger02 Auto
Keyword Property BookShelfTrigger03 Auto
Keyword Property BookShelfTrigger04 Auto
{List of required Keywords}

ObjectReference Property BookMarker01 Auto Hidden
ObjectReference Property BookMarker02 Auto Hidden
ObjectReference Property BookMarker03 Auto Hidden
ObjectReference Property BookMarker04 Auto Hidden
ObjectReference Property BookMarker05 Auto Hidden
ObjectReference Property BookMarker06 Auto Hidden
ObjectReference Property BookMarker07 Auto Hidden
ObjectReference Property BookMarker08 Auto Hidden
ObjectReference Property BookMarker09 Auto Hidden
ObjectReference Property BookMarker10 Auto Hidden
ObjectReference Property BookMarker11 Auto Hidden
ObjectReference Property BookMarker12 Auto Hidden
ObjectReference Property BookMarker13 Auto Hidden
ObjectReference Property BookMarker14 Auto Hidden
ObjectReference Property BookMarker15 Auto Hidden
ObjectReference Property BookMarker16 Auto Hidden
ObjectReference Property BookMarker17 Auto Hidden
ObjectReference Property BookMarker18 Auto Hidden
ObjectReference Property BookShelfTrigger01Ref Auto Hidden
ObjectReference Property BookShelfTrigger02Ref Auto Hidden
ObjectReference Property BookShelfTrigger03Ref Auto Hidden
ObjectReference Property BookShelfTrigger04Ref Auto Hidden

Int Property MaxBooksAllowed Auto Hidden
{Max books allowed on this partciular shelf}

Int Property CurrentBookAmount Auto Hidden
{The current amount of books placed on the shelf}

Form Property EmptyForm Auto Hidden
{Null Form}

ObjectReference Property EmptyRef Auto Hidden
{Null Ref}

Form Property CurrentBookForm Auto Hidden
{Book Form we are working with at any one time}

ObjectReference Property CurrentBookRef Auto Hidden
{Book Ref we are working with at any one time}

Form Property PlacedBook01 Auto Hidden
Form Property PlacedBook02 Auto Hidden
Form Property PlacedBook03 Auto Hidden
Form Property PlacedBook04 Auto Hidden
Form Property PlacedBook05 Auto Hidden
Form Property PlacedBook06 Auto Hidden
Form Property PlacedBook07 Auto Hidden
Form Property PlacedBook08 Auto Hidden
Form Property PlacedBook09 Auto Hidden
Form Property PlacedBook10 Auto Hidden
Form Property PlacedBook11 Auto Hidden
Form Property PlacedBook12 Auto Hidden
Form Property PlacedBook13 Auto Hidden
Form Property PlacedBook14 Auto Hidden
Form Property PlacedBook15 Auto Hidden
Form Property PlacedBook17 Auto Hidden
Form Property PlacedBook16 Auto Hidden
Form Property PlacedBook18 Auto Hidden
{List of Placed Book Forms}

ObjectReference Property PlacedBook01Ref Auto Hidden
ObjectReference Property PlacedBook02Ref Auto Hidden
ObjectReference Property PlacedBook03Ref Auto Hidden
ObjectReference Property PlacedBook04Ref Auto Hidden
ObjectReference Property PlacedBook05Ref Auto Hidden
ObjectReference Property PlacedBook06Ref Auto Hidden
ObjectReference Property PlacedBook07Ref Auto Hidden
ObjectReference Property PlacedBook08Ref Auto Hidden
ObjectReference Property PlacedBook09Ref Auto Hidden
ObjectReference Property PlacedBook10Ref Auto Hidden
ObjectReference Property PlacedBook11Ref Auto Hidden
ObjectReference Property PlacedBook12Ref Auto Hidden
ObjectReference Property PlacedBook13Ref Auto Hidden
ObjectReference Property PlacedBook14Ref Auto Hidden
ObjectReference Property PlacedBook15Ref Auto Hidden
ObjectReference Property PlacedBook17Ref Auto Hidden
ObjectReference Property PlacedBook16Ref Auto Hidden
ObjectReference Property PlacedBook18Ref Auto Hidden
{List of Placed Book Refs}

Bool Property AlreadyLoaded = FALSE Auto Hidden
{Whether this scritp has already went through it's OnCellLoad() Event}

Bool Property BlockBooks = FALSE Auto Hidden
{Used for when you can't place any more books}

Message Property BookShelfFirstActivateMESSAGE Auto
{Display message when the player activates a bookshelf for the first time.  Only displays once.}

Message Property BookShelfNoMoreRoomMESSAGE Auto
{Displayed message for when the amount of books the player is placing excedes the shelf limit.}

Message Property BookShelfNotABookMESSAGE Auto
{Message displayed when the player places a non book form in the container.}

Message Property BookShelfRoomLeftMESSAGE Auto
{Notification that tells the player how much room is left on the shelf upon first activating it.}

GlobalVariable Property BookShelfGlobal Auto
{Global showing whether or not the player has ever activated a bookshelf}



EVENT OnCellLoad()
	if AlreadyLoaded == FALSE
		;Trace("BOOKCASE - Running OnCellLoad()")
		; Get all the book markers
		BookMarker01 = GetLinkedRef(BookShelfBook01)
		BookMarker02 = GetLinkedRef(BookShelfBook02)
		BookMarker03 = GetLinkedRef(BookShelfBook03)
		BookMarker04 = GetLinkedRef(BookShelfBook04)
		BookMarker05 = GetLinkedRef(BookShelfBook05)
		BookMarker06 = GetLinkedRef(BookShelfBook06)
		BookMarker07 = GetLinkedRef(BookShelfBook07)
		BookMarker08 = GetLinkedRef(BookShelfBook08)
		BookMarker09 = GetLinkedRef(BookShelfBook09)
		BookMarker10 = GetLinkedRef(BookShelfBook10)
		BookMarker11 = GetLinkedRef(BookShelfBook11)
		BookMarker12 = GetLinkedRef(BookShelfBook12)
		BookMarker13 = GetLinkedRef(BookShelfBook13)
		BookMarker14 = GetLinkedRef(BookShelfBook14)
		BookMarker15 = GetLinkedRef(BookShelfBook15)
		BookMarker16 = GetLinkedRef(BookShelfBook16)
		BookMarker17 = GetLinkedRef(BookShelfBook17)
		BookMarker18 = GetLinkedRef(BookShelfBook18)
		BookShelfTrigger01Ref = (GetLinkedRef(BookShelfTrigger01) as PlayerBookShelfTriggerSCRIPT)
		BookShelfTrigger02Ref = (GetLinkedRef(BookShelfTrigger02) as PlayerBookShelfTriggerSCRIPT)
		BookShelfTrigger03Ref = (GetLinkedRef(BookShelfTrigger03) as PlayerBookShelfTriggerSCRIPT)
		BookShelfTrigger04Ref = (GetLinkedRef(BookShelfTrigger04) as PlayerBookShelfTriggerSCRIPT)
		; Count how many books can be placed on this shelf
		CountMaxBooks()

		AlreadyLoaded = TRUE
	endif

	; Check if this shelf has already been messed up by vanilla bug, and issue a warning in a log.
	; Actual healing process will trigger when all books are removed from the display shelf.
	;if !(CurrentBookAmount >= 0 && CurrentBookAmount <= MaxBooksAllowed && CurrentBookAmount == NumBooksOnShelf())
	;	Trace("BookShelfFix - An inconsistent bookshelf " + self + " found in " + self.GetCurrentLocation() + ", fix will be applied when you pull out all the books from the shelf.")
	;endif
endEVENT


EVENT OnActivate(ObjectReference akActionRef)
	; Removing all items from container as a precaution
	;Trace("BOOKCASE - I've been ACTIVATED!")

	;USLEEP 3.0.1 - Bug #19515
	CleanInvalidPlacedBooks()
	CurrentBookAmount = NumBooksOnShelf()

	BlockActivate()
	;Trace("BOOKCASE - Blocking activate on all books")
	;Trace("BOOKCASE - BookShelfTrigger01Ref = " + BookShelfTrigger01Ref)
	BookShelfRoomLeftMESSAGE.Show((MaxBooksAllowed - CurrentBookAmount))
	;debug.Notification("You can place " + (MaxBooksAllowed - CurrentBookAmount) + " more books on this shelf.")

	if (BookShelfGlobal.GetValue() == 0)
		BookShelfFirstActivateMESSAGE.Show()
		BookShelfGlobal.SetValue(1)
	endif
	;CurrentBookAmount = 0

	if BookShelfTrigger01Ref
		;Trace("BOOKCASE - Setting FIRST trigger to GoToState IgnoreBooks")
		BookShelfTrigger01Ref.GoToState("IgnoreBooks")
	endif
	if BookShelfTrigger02Ref
		;Trace("BOOKCASE - Setting SECOND trigger to GoToState IgnoreBooks")
		BookShelfTrigger02Ref.GoToState("IgnoreBooks")
	endif
	if BookShelfTrigger03Ref
		;Trace("BOOKCASE - Setting THIRD trigger to GoToState IgnoreBooks")
		BookShelfTrigger03Ref.GoToState("IgnoreBooks")
	endif
	if BookShelfTrigger04Ref
		;Trace("BOOKCASE - Setting FOURTH trigger to GoToState IgnoreBooks")
		BookShelfTrigger04Ref.GoToState("IgnoreBooks")
	endif

	While (Utility.IsInMenuMode())
		Utility.Wait(0.25)
	EndWhile
	; The following will fire when the player leaves inventory

	;USLEEP 3.0.1 - Bug #19515
	; When display shelf becomes empty, reset it to fix corrupted shelves
	if NumBooksOnShelf() == 0
		RemoveAllItems(akTransferTo = Game.GetPlayer(), abRemoveQuestItems = true) ; make sure all the books are transfered into player's inventory
		ClearDisplayShelf()
		Wait(0.25) ; wait until RemoveAllItems() is (hopefully) done
		CurrentBookAmount = 0
		;Trace("BookShelfFix - Cleaned up bookshelf " + self + ". This is not an error.")
	endif

	;Trace("BOOKCASE - Out of Inventory so placing all the books")
	UpdateBooks()

	if BookShelfTrigger01Ref
		;Trace("BOOKCASE - Setting FIRST trigger to GoToState WaitForBooks")
		BookShelfTrigger01Ref.GoToState("WaitForBooks")
	endif
	if BookShelfTrigger02Ref
		;Trace("BOOKCASE - Setting SECOND trigger to GoToState WaitForBooks")
		BookShelfTrigger02Ref.GoToState("WaitForBooks")
	endif
	if BookShelfTrigger03Ref
		;Trace("BOOKCASE - Setting THIRD trigger to GoToState WaitForBooks")
		BookShelfTrigger03Ref.GoToState("WaitForBooks")
	endif
	if BookShelfTrigger04Ref
		;Trace("BOOKCASE - Setting FOURTH trigger to GoToState WaitForBooks")
		BookShelfTrigger04Ref.GoToState("WaitForBooks")
	endif
endEVENT


Event OnItemRemoved(Form akBaseItem, int aiItemCount, ObjectReference akItemReference, ObjectReference akDestContainer)
	if (akBaseItem as Book)
		if BlockBooks == FALSE
			; If the item is a book find the corresponding book reference and remove it.
			;Trace("BOOKCASE - Form being Removed " + akBaseItem + " is a Book! Remove it from the list")
			RemoveBooks(akBaseItem, aiItemCount)
			CurrentBookAmount = CurrentBookAmount - aiItemCount
		else
			BlockBooks = FALSE
		endif
	endif
endEvent


Event OnItemAdded(Form akBaseItem, int aiItemCount, ObjectReference akItemReference, ObjectReference akSourceContainer)
	;Trace("BOOKCASE - Adding " + akBaseItem + " to the Book Container")
	
	if (akBaseItem as Book)
		; If the item being added is a book then check to see if there is room in on the shelf.
		;Trace("BOOKCASE - Form being Added " + akBaseItem + " is a Book!")
		if ((aiItemCount + CurrentBookAmount) <= MaxBooksAllowed)
			; There's room on teh shelf, manage the book placement
			;Trace("BOOKCASE - " + CurrentBookAmount + "/" + MaxBooksAllowed + " There is room for another book, lets place it on the shelf.")
			AddBooks(akBaseItem, aiItemCount)
			CurrentBookAmount = CurrentBookAmount + aiItemCount
		else
			; There is no room on the shelf.  Tell the player this and give him his book back.
			;Trace("BOOKCASE - " + CurrentBookAmount + "/" + MaxBooksAllowed + " There's no more room for books on this shelf.")
			utility.waitMenuMode(0)
			;MessageBox("You can't place that many books on this shelf")
			BookShelfNoMoreRoomMESSAGE.Show()
			;Trace("BOOKCASE - Remove it from this container...")
			BlockBooks = TRUE
			self.RemoveItem(akBaseItem, aiItemCount, true, Game.GetPlayer())
			;Trace("BOOKCASE - ...and give it back to the player")
		endif
	else
		; The item placed in the container isn't a book, so give the player back and tell him only books are allowed.
		;Trace("BOOKCASE - Form " + akBaseItem + " is NOT a Book!")
		;Trace("BOOKCASE - Since the placed item wasn't a book remove it from the container...")

		self.RemoveItem(akBaseItem, aiItemCount, true, Game.GetPlayer())
		;Trace("BOOKCASE - ...and give it back to the player")

		;(Game.GetPlayer()).AddItem(akBaseItem, aiItemCount, True)
		utility.WaitMenuMode(0)
		;MessageBox("You can only place books on this shelf")
		BookShelfNotABookMESSAGE.Show()
	endif
	
endEvent

Function BlockActivate()
	if Placedbook01Ref
		PlacedBook01Ref.BlockActivation(TRUE)
	endif
	if Placedbook02Ref
		PlacedBook02Ref.BlockActivation(TRUE)
	endif
	if Placedbook03Ref
		PlacedBook03Ref.BlockActivation(TRUE)
	endif
	if Placedbook04Ref
		PlacedBook04Ref.BlockActivation(TRUE)
	endif
	if Placedbook05Ref
		PlacedBook05Ref.BlockActivation(TRUE)
	endif
	if Placedbook06Ref
		PlacedBook06Ref.BlockActivation(TRUE)
	endif
	if Placedbook07Ref
		PlacedBook07Ref.BlockActivation(TRUE)
	endif
	if Placedbook08Ref
		PlacedBook08Ref.BlockActivation(TRUE)
	endif
	if Placedbook09Ref
		PlacedBook09Ref.BlockActivation(TRUE)
	endif
	if Placedbook10Ref
		PlacedBook10Ref.BlockActivation(TRUE)
	endif
	if Placedbook11Ref
		PlacedBook11Ref.BlockActivation(TRUE)
	endif
	if Placedbook12Ref
		PlacedBook12Ref.BlockActivation(TRUE)
	endif
	if Placedbook13Ref
		PlacedBook13Ref.BlockActivation(TRUE)
	endif
	if Placedbook14Ref
		PlacedBook14Ref.BlockActivation(TRUE)
	endif
	if Placedbook15Ref
		PlacedBook15Ref.BlockActivation(TRUE)
	endif
	if Placedbook16Ref
		PlacedBook16Ref.BlockActivation(TRUE)
	endif
	if Placedbook17Ref
		PlacedBook17Ref.BlockActivation(TRUE)
	endif
	if Placedbook18Ref
		PlacedBook18Ref.BlockActivation(TRUE)
	endif
	;Trace("BOOKCASE - All book activation has been blocked")
endFunction

Function UnBlockActivate()
	if Placedbook01Ref
		PlacedBook01Ref.BlockActivation(FALSE)
	endif
	if Placedbook02Ref
		PlacedBook02Ref.BlockActivation(FALSE)
	endif
	if Placedbook03Ref
		PlacedBook03Ref.BlockActivation(FALSE)
	endif
	if Placedbook04Ref
		PlacedBook04Ref.BlockActivation(FALSE)
	endif
	if Placedbook05Ref
		PlacedBook05Ref.BlockActivation(FALSE)
	endif
	if Placedbook06Ref
		PlacedBook06Ref.BlockActivation(FALSE)
	endif
	if Placedbook07Ref
		PlacedBook07Ref.BlockActivation(FALSE)
	endif
	if Placedbook08Ref
		PlacedBook08Ref.BlockActivation(FALSE)
	endif
	if Placedbook09Ref
		PlacedBook09Ref.BlockActivation(FALSE)
	endif
	if Placedbook10Ref
		PlacedBook10Ref.BlockActivation(FALSE)
	endif
	if Placedbook11Ref
		PlacedBook11Ref.BlockActivation(FALSE)
	endif
	if Placedbook12Ref
		PlacedBook12Ref.BlockActivation(FALSE)
	endif
	if Placedbook13Ref
		PlacedBook13Ref.BlockActivation(FALSE)
	endif
	if Placedbook14Ref
		PlacedBook14Ref.BlockActivation(FALSE)
	endif
	if Placedbook15Ref
		PlacedBook15Ref.BlockActivation(FALSE)
	endif
	if Placedbook16Ref
		PlacedBook16Ref.BlockActivation(FALSE)
	endif
	if Placedbook17Ref
		PlacedBook17Ref.BlockActivation(FALSE)
	endif
	if Placedbook18Ref
		PlacedBook18Ref.BlockActivation(FALSE)
	endif
	;Trace("BOOKCASE - All book activation has been UNblocked")
endFunction


Function RemoveBooks(Form BookBase, Int BookAmount)
	; Find the removed book form(s) and remove it from the display

	While BookAmount > 0
		if PlacedBook01 == BookBase
			;Trace("BOOKCASE - PlacedBook01 matches, Removing this book")
			PlacedBook01 = EmptyForm
		elseif PlacedBook02 == BookBase
			;Trace("BOOKCASE - PlacedBook02 matches, Removing this book")
			PlacedBook02 = EmptyForm
		elseif PlacedBook03 == BookBase
			;Trace("BOOKCASE - PlacedBook03 matches, Removing this book")
			PlacedBook03 = EmptyForm
		elseif PlacedBook04 == BookBase
			;Trace("BOOKCASE - PlacedBook04 matches, Removing this book")
			PlacedBook04 = EmptyForm
		elseif PlacedBook05 == BookBase
			;Trace("BOOKCASE - PlacedBook05 matches, Removing this book")
			PlacedBook05 = EmptyForm
		elseif PlacedBook06 == BookBase
			;Trace("BOOKCASE - PlacedBook06 matches, Removing this book")
			PlacedBook06 = EmptyForm
		elseif PlacedBook07 == BookBase
			;Trace("BOOKCASE - PlacedBook07 matches, Removing this book")
			PlacedBook07 = EmptyForm
		elseif PlacedBook08 == BookBase
			;Trace("BOOKCASE - PlacedBook08 matches, Removing this book")
			PlacedBook08 = EmptyForm
		elseif PlacedBook09 == BookBase
			;Trace("BOOKCASE - PlacedBook09 matches, Removing this book")
			PlacedBook09 = EmptyForm
		elseif PlacedBook10 == BookBase
			;Trace("BOOKCASE - PlacedBook10 matches, Removing this book")
			PlacedBook10 = EmptyForm
		elseif PlacedBook11 == BookBase
			;Trace("BOOKCASE - PlacedBook11 matches, Removing this book")
			PlacedBook11 = EmptyForm
		elseif PlacedBook12 == BookBase
			;Trace("BOOKCASE - PlacedBook12 matches, Removing this book")
			PlacedBook12 = EmptyForm
		elseif PlacedBook13 == BookBase
			;Trace("BOOKCASE - PlacedBook13 matches, Removing this book")
			PlacedBook13 = EmptyForm
		elseif PlacedBook14 == BookBase
			;Trace("BOOKCASE - PlacedBook14 matches, Removing this book")
			PlacedBook14 = EmptyForm
		elseif PlacedBook15 == BookBase
			;Trace("BOOKCASE - PlacedBook15 matches, Removing this book")
			PlacedBook15 = EmptyForm
		elseif PlacedBook16 == BookBase
			;Trace("BOOKCASE - PlacedBook16 matches, Removing this book")
			PlacedBook16 = EmptyForm
		elseif PlacedBook17 == BookBase
			;Trace("BOOKCASE - PlacedBook17 matches, Removing this book")
			PlacedBook17 = EmptyForm
		elseif PlacedBook18 == BookBase
			;Trace("BOOKCASE - PlacedBook18 matches, Removing this book")
			PlacedBook18 = EmptyForm
		endif

					BookAmount = BookAmount - 1

	endWhile
endFunction

Function AddBooks(Form BookBase, Int BookAmount)
	; Find an empty book form and place the new book there
	While BookAmount > 0
		if PlacedBook01 == EmptyForm
			;Trace("BOOKCASE - PlacedBook01 is empty, placing book there")
			PlacedBook01 = BookBase
		elseif PlacedBook02 == EmptyForm
			;Trace("BOOKCASE - PlacedBook02 is empty, placing book there")
			PlacedBook02 = BookBase
		elseif PlacedBook03 == EmptyForm
			;Trace("BOOKCASE - PlacedBook03 is empty, placing book there")
			PlacedBook03 = BookBase
		elseif PlacedBook04 == EmptyForm
			;Trace("BOOKCASE - PlacedBook04 is empty, placing book there")
			PlacedBook04 = BookBase
		elseif PlacedBook05 == EmptyForm
			;Trace("BOOKCASE - PlacedBook05 is empty, placing book there")
			PlacedBook05 = BookBase
		elseif PlacedBook06 == EmptyForm
			;Trace("BOOKCASE - PlacedBook06 is empty, placing book there")
			PlacedBook06 = BookBase
		elseif PlacedBook07 == EmptyForm
			;Trace("BOOKCASE - PlacedBook07 is empty, placing book there")
			PlacedBook07 = BookBase
		elseif PlacedBook08 == EmptyForm
			;Trace("BOOKCASE - PlacedBook08 is empty, placing book there")
			PlacedBook08 = BookBase
		elseif PlacedBook09 == EmptyForm
			;Trace("BOOKCASE - PlacedBook09 is empty, placing book there")
			PlacedBook09 = BookBase
		elseif PlacedBook10 == EmptyForm
			;Trace("BOOKCASE - PlacedBook10 is empty, placing book there")
			PlacedBook10 = BookBase
		elseif PlacedBook11 == EmptyForm
			;Trace("BOOKCASE - PlacedBook11 is empty, placing book there")
			PlacedBook11 = BookBase
		elseif PlacedBook12 == EmptyForm
			;Trace("BOOKCASE - PlacedBook12 is empty, placing book there")
			PlacedBook12 = BookBase
		elseif PlacedBook13 == EmptyForm
			;Trace("BOOKCASE - PlacedBook13 is empty, placing book there")
			PlacedBook13 = BookBase
		elseif PlacedBook14 == EmptyForm
			;Trace("BOOKCASE - PlacedBook14 is empty, placing book there")
			PlacedBook14 = BookBase
		elseif PlacedBook15 == EmptyForm
			;Trace("BOOKCASE - PlacedBook15 is empty, placing book there")
			PlacedBook15 = BookBase
		elseif PlacedBook16 == EmptyForm
			;Trace("BOOKCASE - PlacedBook16 is empty, placing book there")
			PlacedBook16 = BookBase
		elseif PlacedBook17 == EmptyForm
			;Trace("BOOKCASE - PlacedBook17 is empty, placing book there")
			PlacedBook17 = BookBase
		elseif PlacedBook18 == EmptyForm
			;Trace("BOOKCASE - PlacedBook18 is empty, placing book there")
			PlacedBook18 = BookBase
		endif

					BookAmount = BookAmount - 1

	endWhile
endFunction


Function CountMaxBooks()
	; Checks how many books can be placed on this shelf
	if BookMarker01 == EmptyRef
		MaxBooksAllowed = 0
	elseif BookMarker02 == EmptyRef
		MaxBooksAllowed = 1
	elseif BookMarker03 == EmptyRef
		MaxBooksAllowed = 2
	elseif BookMarker04 == EmptyRef
		MaxBooksAllowed = 3
	elseif BookMarker05 == EmptyRef
		MaxBooksAllowed = 4
	elseif BookMarker06 == EmptyRef
		MaxBooksAllowed = 5
	elseif BookMarker07 == EmptyRef
		MaxBooksAllowed = 6
	elseif BookMarker08 == EmptyRef
		MaxBooksAllowed = 7
	elseif BookMarker09 == EmptyRef
		MaxBooksAllowed = 8
	elseif BookMarker10 == EmptyRef
		MaxBooksAllowed = 9
	elseif BookMarker11 == EmptyRef
		MaxBooksAllowed = 10
	elseif BookMarker12 == EmptyRef
		MaxBooksAllowed = 11
	elseif BookMarker13 == EmptyRef
		MaxBooksAllowed = 12
	elseif BookMarker14 == EmptyRef
		MaxBooksAllowed = 13
	elseif BookMarker15 == EmptyRef
		MaxBooksAllowed = 14
	elseif BookMarker16 == EmptyRef
		MaxBooksAllowed = 15
	elseif BookMarker17 == EmptyRef
		MaxBooksAllowed = 16
	elseif BookMarker18 == EmptyRef
		MaxBooksAllowed = 17
	elseif BookMarker18
		MaxBooksAllowed = 18
	endif
	;Trace("BOOKCASE - " + MaxBooksAllowed + " books can be placed on this shelf")
endFunction


ObjectReference Function UpdateSingleBook(Form TargetBook, ObjectReference PlacedBookRef, ObjectReference TargetMarker)
	ObjectReference retVal ; We return the book we placed (or None)
	; Note - it would be more efficient to move the book to its home position if the desired
	; book matches the placed book, but MoveTo doesn't work correctly with multi-part dynamic
	; objects. So we sidestep the issue by always deleting and placing
	;USLEEP 3.0.1 - Bug #19515 added form ID check here.
	if PlacedBookRef && PlacedBookRef.GetFormID()
		PlacedBookRef.Disable()
		PlacedBookRef.Delete()
	endIf
	if TargetBook
		retVal = TargetMarker.PlaceAtMe(TargetBook)
		retVal.BlockActivation()
	endIf
	return retVal
EndFunction


Function UpdateBooks()
	GoToState("PlacingBooks") ; Future calls should not mess with this stuff
	
	PlacedBook01Ref = UpdateSingleBook(PlacedBook01, PlacedBook01Ref, BookMarker01)
	PlacedBook02Ref = UpdateSingleBook(PlacedBook02, PlacedBook02Ref, BookMarker02)
	PlacedBook03Ref = UpdateSingleBook(PlacedBook03, PlacedBook03Ref, BookMarker03)
	PlacedBook04Ref = UpdateSingleBook(PlacedBook04, PlacedBook04Ref, BookMarker04)
	PlacedBook05Ref = UpdateSingleBook(PlacedBook05, PlacedBook05Ref, BookMarker05)
	
	PlacedBook06Ref = UpdateSingleBook(PlacedBook06, PlacedBook06Ref, BookMarker06)
	PlacedBook07Ref = UpdateSingleBook(PlacedBook07, PlacedBook07Ref, BookMarker07)
	PlacedBook08Ref = UpdateSingleBook(PlacedBook08, PlacedBook08Ref, BookMarker08)
	PlacedBook09Ref = UpdateSingleBook(PlacedBook09, PlacedBook09Ref, BookMarker09)
	PlacedBook10Ref = UpdateSingleBook(PlacedBook10, PlacedBook10Ref, BookMarker10)
	
	PlacedBook11Ref = UpdateSingleBook(PlacedBook11, PlacedBook11Ref, BookMarker11)
	PlacedBook12Ref = UpdateSingleBook(PlacedBook12, PlacedBook12Ref, BookMarker12)
	PlacedBook13Ref = UpdateSingleBook(PlacedBook13, PlacedBook13Ref, BookMarker13)
	PlacedBook14Ref = UpdateSingleBook(PlacedBook14, PlacedBook14Ref, BookMarker14)
	PlacedBook15Ref = UpdateSingleBook(PlacedBook15, PlacedBook15Ref, BookMarker15)
	
	PlacedBook16Ref = UpdateSingleBook(PlacedBook16, PlacedBook16Ref, BookMarker16)
	PlacedBook17Ref = UpdateSingleBook(PlacedBook17, PlacedBook17Ref, BookMarker17)
	PlacedBook18Ref = UpdateSingleBook(PlacedBook18, PlacedBook18Ref, BookMarker18)
	
	UnBlockActivate() ; Allow the player to mess with them
	GoToState("") ; Now allow books to be updated again
EndFunction


State PlacingBooks
	Function UpdateBooks()
		; Already updating books, so ignore
	EndFunction
EndState


;
; ;USLEEP 3.0.1 - Bug #19515: Everything past this point is new stuff to support fixing the bookshelf containers.
;
Function PickUp(ObjectReference BookRef)
	;Trace("BookShelfFix " + self + " PickUp() - " + BookRef + ", Base = " + BookRef.GetBaseObject())

	if (BookRef && BookRef.GetBaseObject() as Book)

		; Remove a display book first
		if RemoveBookByRef(BookRef)

			GoToState("PickingUp")
			; Then remove a book in container
			self.RemoveItem(BookRef.GetBaseObject(), 1)
			Wait(0.25) ; wait for OnItemRemoved() event to fire

			GoToState("")
		endif
	endIf
EndFunction


State PickingUp
	; Used when a book has been picked up directly from the display

	Event OnBeginState() ; You Shall Not Pass!
		self.BlockActivation(True)
		BlockActivate()
		;Trace("BookShelfFix " + self + " PickingUp State - Entered State")
	EndEvent

	Event OnActivate(ObjectReference akActionRef)
	EndEvent

	Event OnItemRemoved(Form akBaseItem, int aiItemCount, ObjectReference akItemReference, ObjectReference akDestContainer)
		;Trace("BookShelfFix " + self + " PickingUp State - Item " + akBaseItem + " removed from shelf inventory")
		CurrentBookAmount -= aiItemCount
	EndEvent

	Event OnEndState()
		;Trace("BookShelfFix " + self + " PickingUp State - Leaving State")
		UnblockActivate()
		self.BlockActivation(False)
	EndEvent

EndState


Bool Function RemoveBookByRef(ObjectReference BookRef)
	; If the book had been picked up directly, and if there were multiple copies of it on the shelf,
	; we need to determine the exact location of the book rather than the first copy in a list.
	; Returns True if BookRef has been successfully dismembered from the list or False when the book was missing

	if !BookRef
		return False
	endif

	int BookRefID = BookRef.GetFormID()

	; These references could be a derivative of ObjectReference such as DefaultSetStageOnPlayerAcquireItem
	; and would never match even if they were cast into ObjectReference, so compare their FormIDs rather than comparing them directly.
	if PlacedBook01Ref && PlacedBook01Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook01 matches, Removing this book")
		PlacedBook01 = EmptyForm
		PlacedBook01Ref = EmptyRef
	elseif PlacedBook02Ref && PlacedBook02Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook02 matches, Removing this book")
		PlacedBook02 = EmptyForm
		PlacedBook02Ref = EmptyRef
	elseif PlacedBook03Ref && PlacedBook03Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook03 matches, Removing this book")
		PlacedBook03 = EmptyForm
		PlacedBook03Ref = EmptyRef
	elseif PlacedBook04Ref && PlacedBook04Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook04 matches, Removing this book")
		PlacedBook04 = EmptyForm
		PlacedBook04Ref = EmptyRef
	elseif PlacedBook05Ref && PlacedBook05Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook05 matches, Removing this book")
		PlacedBook05 = EmptyForm
		PlacedBook05Ref = EmptyRef
	elseif PlacedBook06Ref && PlacedBook06Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook06 matches, Removing this book")
		PlacedBook06 = EmptyForm
		PlacedBook06Ref = EmptyRef
	elseif PlacedBook07Ref && PlacedBook07Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook07 matches, Removing this book")
		PlacedBook07 = EmptyForm
		PlacedBook07Ref = EmptyRef
	elseif PlacedBook08Ref && PlacedBook08Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook08 matches, Removing this book")
		PlacedBook08 = EmptyForm
		PlacedBook08Ref = EmptyRef
	elseif PlacedBook09Ref && PlacedBook09Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook09 matches, Removing this book")
		PlacedBook09 = EmptyForm
		PlacedBook09Ref = EmptyRef
	elseif PlacedBook10Ref && PlacedBook10Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook10 matches, Removing this book")
		PlacedBook10 = EmptyForm
		PlacedBook10Ref = EmptyRef
	elseif PlacedBook11Ref && PlacedBook11Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook11 matches, Removing this book")
		PlacedBook11 = EmptyForm
		PlacedBook11Ref = EmptyRef
	elseif PlacedBook12Ref && PlacedBook12Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook12 matches, Removing this book")
		PlacedBook12 = EmptyForm
		PlacedBook12Ref = EmptyRef
	elseif PlacedBook13Ref && PlacedBook13Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook13 matches, Removing this book")
		PlacedBook13 = EmptyForm
		PlacedBook13Ref = EmptyRef
	elseif PlacedBook14Ref && PlacedBook14Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook14 matches, Removing this book")
		PlacedBook14 = EmptyForm
		PlacedBook14Ref = EmptyRef
	elseif PlacedBook15Ref && PlacedBook15Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook15 matches, Removing this book")
		PlacedBook15 = EmptyForm
		PlacedBook15Ref = EmptyRef
	elseif PlacedBook16Ref && PlacedBook16Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook16 matches, Removing this book")
		PlacedBook16 = EmptyForm
		PlacedBook16Ref = EmptyRef
	elseif PlacedBook17Ref && PlacedBook17Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook17 matches, Removing this book")
		PlacedBook17 = EmptyForm
		PlacedBook17Ref = EmptyRef
	elseif PlacedBook18Ref && PlacedBook18Ref.GetFormID() == BookRefID
		;Trace("BookShelfFix RemoveBookByRef() - PlacedBook18 matches, Removing this book")
		PlacedBook18 = EmptyForm
		PlacedBook18Ref = EmptyRef
	else
		return False
	endif

	return True
EndFunction


Int Function NumBooksOnShelf()
	; Count number of books displayed on shelf (not ones kept in inventory)
	int bookCnt = 0

	Form[] PlacedBookAr = new Form[18]

	PlacedBookAr[0]  = PlacedBook01
	PlacedBookAr[1]  = PlacedBook02
	PlacedBookAr[2]  = PlacedBook03
	PlacedBookAr[3]  = PlacedBook04
	PlacedBookAr[4]  = PlacedBook05
	PlacedBookAr[5]  = PlacedBook06
	PlacedBookAr[6]  = PlacedBook07
	PlacedBookAr[7]  = PlacedBook08
	PlacedBookAr[8]  = PlacedBook09
	PlacedBookAr[9]  = PlacedBook10
	PlacedBookAr[10] = PlacedBook11
	PlacedBookAr[11] = PlacedBook12
	PlacedBookAr[12] = PlacedBook13
	PlacedBookAr[13] = PlacedBook14
	PlacedBookAr[14] = PlacedBook15
	PlacedBookAr[15] = PlacedBook16
	PlacedBookAr[16] = PlacedBook17
	PlacedBookAr[17] = PlacedBook18

	int i = 0

	while (i < MaxBooksAllowed && i < 18)
		if PlacedBookAr[i]
			bookCnt += 1
		endif
		i += 1
	endwhile

	; Note that number of books in inventory could NOT match this number due to bugs in vanilla script
	return bookCnt
EndFunction

Function ClearDisplayShelf()
	; Clear BaseID row for display shelf and give back any books in incorrect slots to player

	ObjectReference player = Game.GetPlayer()

	;Trace("BookShelfFix - ClearDisplayShelf()")

	if (MaxBooksAllowed < 1 && PlacedBook01)
		player.AddItem(PlacedBook01, 1, true)
	endif
	if (MaxBooksAllowed < 2 && PlacedBook02)
		player.AddItem(PlacedBook02, 1, true)
	endif
	if (MaxBooksAllowed < 3 && PlacedBook03)
		player.AddItem(PlacedBook03, 1, true)
	endif
	if (MaxBooksAllowed < 4 && PlacedBook04)
		player.AddItem(PlacedBook04, 1, true)
	endif
	if (MaxBooksAllowed < 5 && PlacedBook05)
		player.AddItem(PlacedBook05, 1, true)
	endif
	if (MaxBooksAllowed < 6 && PlacedBook06)
		player.AddItem(PlacedBook06, 1, true)
	endif
	if (MaxBooksAllowed < 7 && PlacedBook07)
		player.AddItem(PlacedBook07, 1, true)
	endif
	if (MaxBooksAllowed < 8 && PlacedBook08)
		player.AddItem(PlacedBook08, 1, true)
	endif
	if (MaxBooksAllowed < 9 && PlacedBook09)
		player.AddItem(PlacedBook09, 1, true)
	endif
	if (MaxBooksAllowed < 10 && PlacedBook10)
		player.AddItem(PlacedBook10, 1, true)
	endif
	if (MaxBooksAllowed < 11 && PlacedBook11)
		player.AddItem(PlacedBook11, 1, true)
	endif
	if (MaxBooksAllowed < 12 && PlacedBook12)
		player.AddItem(PlacedBook12, 1, true)
	endif
	if (MaxBooksAllowed < 13 && PlacedBook13)
		player.AddItem(PlacedBook13, 1, true)
	endif
	if (MaxBooksAllowed < 14 && PlacedBook14)
		player.AddItem(PlacedBook14, 1, true)
	endif
	if (MaxBooksAllowed < 15 && PlacedBook15)
		player.AddItem(PlacedBook15, 1, true)
	endif
	if (MaxBooksAllowed < 16 && PlacedBook16)
		player.AddItem(PlacedBook16, 1, true)
	endif
	if (MaxBooksAllowed < 17 && PlacedBook17)
		player.AddItem(PlacedBook17, 1, true)
	endif
	if (MaxBooksAllowed < 18 && PlacedBook18)
		player.AddItem(PlacedBook18, 1, true)
	endif

	PlacedBook01 = EmptyForm
	PlacedBook02 = EmptyForm
	PlacedBook03 = EmptyForm
	PlacedBook04 = EmptyForm
	PlacedBook05 = EmptyForm
	PlacedBook06 = EmptyForm
	PlacedBook07 = EmptyForm
	PlacedBook08 = EmptyForm
	PlacedBook09 = EmptyForm
	PlacedBook10 = EmptyForm
	PlacedBook11 = EmptyForm
	PlacedBook12 = EmptyForm
	PlacedBook13 = EmptyForm
	PlacedBook14 = EmptyForm
	PlacedBook15 = EmptyForm
	PlacedBook16 = EmptyForm
	PlacedBook17 = EmptyForm
	PlacedBook18 = EmptyForm
EndFunction


Function CleanInvalidPlacedBooks()
	; Clean invalid (nonexistent) forms off PlacedBook**. Invalid forms may be resulted in removed mods.
	; Cleaning them will prevent them from eating up PlacedBook slots permanently.

	if !(PlacedBook01 && PlacedBook01.GetFormID()) ; nonexistent forms are still cast into True, but their ID are always 0
		PlacedBook01 = EmptyForm
	endif
	if !(PlacedBook02 && PlacedBook02.GetFormID())
		PlacedBook02 = EmptyForm
	endif
	if !(PlacedBook03 && PlacedBook03.GetFormID())
		PlacedBook03 = EmptyForm
	endif
	if !(PlacedBook04 && PlacedBook04.GetFormID())
		PlacedBook04 = EmptyForm
	endif
	if !(PlacedBook05 && PlacedBook05.GetFormID())
		PlacedBook05 = EmptyForm
	endif
	if !(PlacedBook06 && PlacedBook06.GetFormID())
		PlacedBook06 = EmptyForm
	endif
	if !(PlacedBook07 && PlacedBook07.GetFormID())
		PlacedBook07 = EmptyForm
	endif
	if !(PlacedBook08 && PlacedBook08.GetFormID())
		PlacedBook08 = EmptyForm
	endif
	if !(PlacedBook09 && PlacedBook09.GetFormID())
		PlacedBook09 = EmptyForm
	endif
	if !(PlacedBook10 && PlacedBook10.GetFormID())
		PlacedBook10 = EmptyForm
	endif
	if !(PlacedBook11 && PlacedBook11.GetFormID())
		PlacedBook11 = EmptyForm
	endif
	if !(PlacedBook12 && PlacedBook12.GetFormID())
		PlacedBook12 = EmptyForm
	endif
	if !(PlacedBook13 && PlacedBook13.GetFormID())
		PlacedBook13 = EmptyForm
	endif
	if !(PlacedBook14 && PlacedBook14.GetFormID())
		PlacedBook14 = EmptyForm
	endif
	if !(PlacedBook15 && PlacedBook15.GetFormID())
		PlacedBook15 = EmptyForm
	endif
	if !(PlacedBook16 && PlacedBook16.GetFormID())
		PlacedBook16 = EmptyForm
	endif
	if !(PlacedBook17 && PlacedBook17.GetFormID())
		PlacedBook17 = EmptyForm
	endif
	if !(PlacedBook18 && PlacedBook18.GetFormID())
		PlacedBook18 = EmptyForm
	endif

EndFunction

;USSEP 4.1.5 Bug #13862 - Added to reset all bookshelf form and reference pointers after the chest has been dumped by the prefabs script.
Function USSEP_ClearAllBookForms()
	PlacedBook01 = EmptyForm
	PlacedBook02 = EmptyForm
	PlacedBook03 = EmptyForm
	PlacedBook04 = EmptyForm
	PlacedBook05 = EmptyForm
	PlacedBook06 = EmptyForm
	PlacedBook07 = EmptyForm
	PlacedBook08 = EmptyForm
	PlacedBook09 = EmptyForm
	PlacedBook10 = EmptyForm
	PlacedBook11 = EmptyForm
	PlacedBook12 = EmptyForm
	PlacedBook13 = EmptyForm
	PlacedBook14 = EmptyForm
	PlacedBook15 = EmptyForm
	PlacedBook16 = EmptyForm
	PlacedBook17 = EmptyForm
	PlacedBook18 = EmptyForm
	PlacedBook01Ref = EmptyRef
	PlacedBook02Ref = EmptyRef
	PlacedBook03Ref = EmptyRef
	PlacedBook04Ref = EmptyRef
	PlacedBook05Ref = EmptyRef
	PlacedBook06Ref = EmptyRef
	PlacedBook07Ref = EmptyRef
	PlacedBook08Ref = EmptyRef
	PlacedBook09Ref = EmptyRef
	PlacedBook10Ref = EmptyRef
	PlacedBook11Ref = EmptyRef
	PlacedBook12Ref = EmptyRef
	PlacedBook13Ref = EmptyRef
	PlacedBook14Ref = EmptyRef
	PlacedBook15Ref = EmptyRef
	PlacedBook16Ref = EmptyRef
	PlacedBook17Ref = EmptyRef
	PlacedBook18Ref = EmptyRef
EndFunction
