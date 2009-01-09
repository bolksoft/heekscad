// PropertyColor.h

#pragma once

#include "Property.h"
#include "HeeksColor.h"

class HeeksObj;

class PropertyColor:public Property{
private:
	wxString title;

public:
	HeeksColor m_initial_value;
	HeeksObj* m_object;
	void(*m_callbackfunc)(HeeksColor, HeeksObj*);

	PropertyColor(const wxChar* t, HeeksColor initial_value, HeeksObj* object, void(*callbackfunc)(HeeksColor, HeeksObj*) = NULL);
	~PropertyColor();

	// Property's virtual functions
	int get_property_type(){return ColorPropertyType;}
	bool property_editable()const{return m_callbackfunc != NULL;}
	Property *MakeACopy(void)const;
	void CallSetFunction()const{ if(m_callbackfunc)(*m_callbackfunc)(m_initial_value, m_object);}
	const wxChar* GetShortString(void)const;
};
