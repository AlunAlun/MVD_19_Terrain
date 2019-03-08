#pragma once
#include "includes.h"
#include "Components.h"
#include <vector>
#include "ControlSystem.h"


//Forward declare ControlSystem to get input
class ControlSystem; 

class Script {
public:
	//constructor - must pass entity owner
	Script(int owner) { owner_ = owner;};
	
	//virtual function allows optional override
	virtual void init() {};

	// pure virtual functions FORCES execution of functions in derived classes
	virtual void update(float dt) = 0;

	//sets pointer to control system
	void setInput(ControlSystem* cont_sys) { input_ = cont_sys; };

protected:
	int owner_; //id of entity which owns this script
	ControlSystem* input_ = nullptr; //pointer to control system
};

class ScriptSystem {
public:
	
	//initialize by setting control system pointer to send to scripts
	void init(ControlSystem* cont_sys) { input_ = cont_sys; };

	//lateInit calls init of all registered scripts
	void lateInit();

	//update all scripts 
	void update(float dt);
	
	//register new script
	void registerScript(Script* new_script);

private:
	std::vector<Script*> scripts_;

	//pointer to the control system
	ControlSystem* input_;


};




