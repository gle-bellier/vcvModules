#include "plugin.hpp"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

struct Wave : Module {
	enum ParamId {
		PITCH_PARAM,
		DIST_PARAM,
		SHAPE_PARAM,
		FINE_PARAM,
		FM_PARAM,
		AN_PARAM,
		NUM_PARAMS

	};
	enum InputId {
		PITCH_INPUT,
		CVDIST_INPUT,
		CVSHAPE_INPUT,
		FM_INPUT,
		NUM_INPUTS
	};
	enum OutputId {
		ANL_OUTPUT,
		DGT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightId {
		//BLINK_LIGHT,
		NUM_LIGHTS
	};
	float phase = 0.f;
	float blinkPhase = 0.f;

	Wave() {
		// Configure the module
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		// Configure parameters
		// See engine/Param.hpp for config() arguments
		configParam(PITCH_PARAM, -1.f, 1.f, 0.f, "Pitch", " Hz", 2.f, dsp::FREQ_C4);
		configParam(FINE_PARAM, -1.f, 1.f, 0.f, "Fine");
		configParam(SHAPE_PARAM, 0.f, 20.f, 0.f, "Shape", "Type");
		configParam(DIST_PARAM, 0.f, 10.f, 0.f, "Dist", " Amp");
		configParam(FM_PARAM, 0.f, 1.f, 0.f, "FM modulation");
	}
	void process(const ProcessArgs &args) override {
		// get value of parameter
		float frequency = params[PITCH_PARAM].getValue();
		float fine = params[FINE_PARAM].getValue();
		float shape = params[SHAPE_PARAM].getValue();
		float distAmount = params[DIST_PARAM].getValue(); //
		float fmAmount = params[FM_PARAM].getValue();

		float pitch = inputs[PITCH_INPUT].getVoltage();
		float shapeCV = inputs[CVSHAPE_INPUT].getVoltage();
		float distCV = inputs[CVDIST_INPUT].getVoltage();
		float fmIn = inputs[FM_INPUT].getVoltage();


		// Compute the frequency from the pitch parameter and input


		// The default pitch is C4 = 261.6256f
		pitch +=frequency;
		if (inputs[FM_INPUT].isConnected()) {
			pitch += fmIn*fmAmount;
		}

		float freq = dsp::FREQ_C4 * std::pow(2.f, pitch)* std::pow(2.f, fine/10.f);

		// Accumulate the phase
		phase += freq * args.sampleTime;
		if (phase >= 0.5f)
			phase -= 1.f;

		// Compute the sine output and shape parameter
		float outSignal = 5*std::sin(2.f * M_PI * phase);
		shape = 0.5f*clamp(shape + shapeCV - 5.f, 0.f,10.f); // -5 because of uni mode LFO delivering 0-10
		outSignal+= shape*std::sin(1.f * M_PI * phase);
		outSignal = outSignal * (1.f/(1.f + shape/5.f)); // in order to normalize the output to -5V/5V

		//want to add some distorsion

		distAmount = 0.5f*clamp(distAmount+distCV,0.f,10.f); // distAmount included between 0 and 5
		float thold = 5.f-(5.f/3.f)*distAmount; // times (5/3) because we want to use it to make pulse width parameter

		float anOut = outSignal;
		float dgtOut = outSignal;

		float d = abs(outSignal);
		if (d >= thold){
				dgtOut = 5.f * outSignal/d;
				anOut = (outSignal/d) * (4.4f  + 0.20f * (std::sin(5.f * M_PI * phase)+std::sin(7.f * M_PI * phase)+std::sin(7.f * M_PI * phase)));
			}
		if (thold<0){
			float psPhase = phase+0.5f;
			if (psPhase >= 0.5f)
				psPhase -= 1.f;
			float pulseWidth = 0.6f-0.20f*distAmount;
			if (psPhase>=pulseWidth and outSignal>=0){			//modification of output signal when above pulsewidth
				dgtOut = -dgtOut;
				anOut = anOut-9.f;
			}
		}
		outputs[ANL_OUTPUT].setVoltage(anOut);
		outputs[DGT_OUTPUT].setVoltage(dgtOut);
		/*Blink light at 1Hz
		blinkPhase += args.sampleTime;
		if (blinkPhase >= 1.f)
			blinkPhase -= 1.f;
		lights[BLINK_LIGHT].setBrightness(blinkPhase < 0.5f ? 1.f : 0.f);*/
	}
};


struct WaveWidget : ModuleWidget {
	WaveWidget(Wave *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Wave.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParam<RoundHugeBlackKnob>(Vec(45, 55), module, Wave::PITCH_PARAM));
		addParam(createParam<Rogan1PWhite>(Vec(25, 152), module, Wave::FINE_PARAM));
		addParam(createParam<Rogan1PWhite>(Vec(93, 152), module, Wave::FM_PARAM));

		addParam(createParam<Rogan1PWhite>(Vec(93, 202), module, Wave::SHAPE_PARAM));
		addParam(createParam<Rogan1PWhite>(Vec(25, 202), module, Wave::DIST_PARAM));


		addInput(createInput<PJ301MPort>(Vec(28, 270), module, Wave::CVDIST_INPUT));
		addInput(createInput<PJ301MPort>(Vec(63, 270), module, Wave::FM_INPUT));
		addInput(createInput<PJ301MPort>(Vec(98, 270), module, Wave::CVSHAPE_INPUT));

		addInput(createInput<PJ301MPort>(Vec(28, 320), module, Wave::PITCH_INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(63, 320), module, Wave::DGT_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(98, 320), module, Wave::ANL_OUTPUT));

		//addChild(createLight<MediumLight<RedLight>>(Vec(41, 59), module, Wave::BLINK_LIGHT));
	}
};


// Define the Model with the Module type, ModuleWidget type, and module slug
Model *modelWave = createModel<Wave, WaveWidget>("Wave");
