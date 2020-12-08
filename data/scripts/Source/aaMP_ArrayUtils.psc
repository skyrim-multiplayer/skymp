Scriptname aaMP_ArrayUtils   

Form[] function add(Form[] forms, Form value) global
	Form[] newForms = Utility.createFormArray(forms.length + 1)
	int i = 0
	while (i < forms.length - 1)
		newForms[i] = forms[i]
		i += 1
	endwhile
	newForms[forms.length] = value
	return newForms
endfunction


Form[] function remove(Form[] forms, Form value) global
	Form[] newForms = Utility.createFormArray(forms.length - 1)
	int i = 0
	int delta = 0
	while (i < forms.length - 1)
		if(forms[i] != value)
			newForms[i - delta] = forms[i]
		else
			delta += 1
		endif
		i += 1
	endwhile
	return newForms
endfunction


Form[] function trim(Form[] forms, int newLength) global
	if(newLength <= 0)
		return none
	endif
	Form[] newForms = Utility.createFormArray(newLength)
	int i = 0
	int delta = 0
	while (i < forms.length - 1)
		if(forms[i] != none)
			newForms[i - delta] = forms[i]
		else
			delta += 1
		endif
		i += 1
	endwhile
	return newForms
endfunction

