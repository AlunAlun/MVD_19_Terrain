#include "ScriptSystem.h"

//call init function of all registered scripts
void ScriptSystem::lateInit() {
	//init all scripts
	for (auto scr : scripts_)
		scr->init();
}

//update all scripts
void ScriptSystem::update(float dt) {
	for (auto scr : scripts_)
		scr->update(dt);
}

//register new script and pass script a pointer to control system
void ScriptSystem::registerScript(Script* new_script) {
	scripts_.push_back(new_script); //add to list
	new_script->setInput(input_); //tell script where control system is

}

