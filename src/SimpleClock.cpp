#include "JWModules.hpp"
#include "dsp/digital.hpp"

struct SimpleClock : Module {
	enum ParamIds {
		CLOCK_PARAM,
		RUN_PARAM,
		PROB_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		GATES_OUTPUT,
		RESET_OUTPUT,
		NUM_OUTPUTS
	};

	bool running = true;
	bool rndReset = false;
	SchmittTrigger runningTrigger;
	float runningLight = 0.0;
	
	float phase = 0.0;
	PulseGenerator gatePulse;
	PulseGenerator resetPulse;

	SimpleClock() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {}
	void step();

	json_t *toJson() {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "running", json_boolean(running));
		return rootJ;
	}

	void fromJson(json_t *rootJ) {
		json_t *runningJ = json_object_get(rootJ, "running");
		if (runningJ)
			running = json_is_true(runningJ);
	}

	void initialize() {
	}

	void randomize() {
	}
};


void SimpleClock::step() {
	if (runningTrigger.process(params[RUN_PARAM].value)) {
		running = !running;
		if(running){
			resetPulse.trigger(0.01);
		}
	}
	runningLight = running ? 1.0 : 0.0;

	bool nextStep = false;
	if (running) {
		float clockTime = powf(2.0, params[CLOCK_PARAM].value);
		phase += clockTime / gSampleRate;
		if (phase >= 1.0) {
			phase -= 1.0;
			nextStep = true;
		}
	}

	if (nextStep) {
		float probScaled = rescalef(params[PROB_PARAM].value, -2, 6, 0, 1);
		if(randomf() < probScaled){
			resetPulse.trigger(0.01);
		}
		gatePulse.trigger(1e-3);
	}

	bool gpulse = gatePulse.process(1.0 / gSampleRate);
	bool rpulse = resetPulse.process(1.0 / gSampleRate);
	outputs[RESET_OUTPUT].value = running && rpulse ? 10.0 : 0.0;
	outputs[GATES_OUTPUT].value = running && gpulse ? 10.0 : 0.0;
}


SimpleClockWidget::SimpleClockWidget() {
	SimpleClock *module = new SimpleClock();
	setModule(module);
	box.size = Vec(15*4, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/SimpleClock.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	addParam(createParam<Davies1900hSmallBlackKnob>(Vec(17, 40), module, SimpleClock::CLOCK_PARAM, -2.0, 6.0, 2.0));
	addOutput(createOutput<PJ301MPort>(Vec(18, 78), module, SimpleClock::GATES_OUTPUT));
	addParam(createParam<LEDButton>(Vec(21, 130), module, SimpleClock::RUN_PARAM, 0.0, 1.0, 0.0));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(26, 130+5), &module->runningLight));

	addOutput(createOutput<PJ301MPort>(Vec(18, 223), module, SimpleClock::RESET_OUTPUT));
	addParam(createParam<Davies1900hSmallBlackKnob>(Vec(16, 305), module, SimpleClock::PROB_PARAM, -2.0, 6.0, -2));
}
