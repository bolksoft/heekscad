// PropertyInt.cpp

#include "stdafx.h"

#include "PropertyInt.h"

PropertyInt::PropertyInt(const wxChar* t, int initial_value, HeeksObj* object, void(*callbackfunc)(int, HeeksObj*)):Property(){
	m_initial_value = initial_value;
	m_object = object;
	m_callbackfunc = callbackfunc;
	title = wxString(t);
}

PropertyInt::~PropertyInt(){
}

const wxChar* PropertyInt::GetShortString(void)const{
	return title.c_str();
}

Property *PropertyInt::MakeACopy(void)const{
	PropertyInt* new_object = new PropertyInt(*this);
	return new_object;
}
