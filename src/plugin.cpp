#include "plugin.hpp"


Plugin *pluginInstance;


void init(Plugin *p) {
	pluginInstance = p;
	//p->slug = TOSTRING(SLUG);
	// Add all Models defined throughout the plugin
	p->addModel(modelWave);
		// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}