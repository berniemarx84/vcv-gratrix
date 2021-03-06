#include "Gratrix.hpp"

enum Spec
{
	LO_BEGIN = -5,    // C-1
	LO_END   =  5,    // C+9
	LO_SIZE  = LO_END - LO_BEGIN + 1,
	E        = 12,    // ET
	N        = 12,    // Number of note outputs
	T        = 2,
	M        = 2*T+1  // Number of octave outputs
};


struct Octave : Module
{
	enum ParamIds
	{
		NUM_PARAMS
	};

	enum InputIds
	{
		IN_INPUT   = 0,
		NUM_INPUTS = IN_INPUT + 1
	};

	enum OutputIds
	{
		NOTE_OUTPUT = 0,
		OCT_OUTPUT  = NOTE_OUTPUT + N,
		NUM_OUTPUTS = OCT_OUTPUT  + M
	};
	enum LightIds {
		KEY_LIGHT  = 0,
		OCT_LIGHT  = KEY_LIGHT + E,
		NUM_LIGHTS = OCT_LIGHT + LO_SIZE
	};

	Octave()
	:
		Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS)
	{}

	void step() override
	{
		static constexpr float e = static_cast<float>(E);
		static constexpr float s = 1.0f / e;

		float in    = inputs[IN_INPUT].value;
		float fnote = std::floor(in * e + 0.5f);
		float qin   = fnote * s;

		int   note = static_cast<int>(fnote);
		int   safe = note + (E * 1000);  // push away from negative numbers
		int   key  = safe % E;
		int   oct  = safe / E;
		float led  = (oct & 1) ? -1.0f : 1.0f;
		      oct -= 1000;

		// quant
		in = qin;

		for (std::size_t i=0; i<N; ++i)
		{
			outputs[i + NOTE_OUTPUT].value = in + i * s;
		}

		for (std::size_t i=0; i<M; ++i)
		{
			outputs[i + OCT_OUTPUT].value = (in - T) + i;
		}

		// Lights

		for (auto &light : lights) light.value = 0.0f;

		lights[KEY_LIGHT + key].value = led;

		if (LO_BEGIN <= oct && oct <= LO_END)
		{
			lights[OCT_LIGHT + oct - LO_BEGIN].value = led;
		}
	}
};


int x(std::size_t i, double radius) { return static_cast<int>(6*15     + 0.5 + radius * dx(i, E)); }
int y(std::size_t i, double radius) { return static_cast<int>(-20+206  + 0.5 + radius * dy(i, E)); }


OctaveWidget::OctaveWidget()
{
	GTX__WIDGET();

	Octave *module = new Octave();
	setModule(module);
	box.size = Vec(12*15, 380);

//	double r1 = 30;
	double r2 = 55;

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "build/res/Octave.svg"), box.size, "OCTAVE");

		pg.circle(Vec(x(0, 0), y(0, 0)), r2+16, "fill:#7092BE;stroke:none");
		pg.circle(Vec(x(0, 0), y(0, 0)), r2-16, "fill:#CEE1FD;stroke:none");

/*		// Wires
		for (std::size_t i=0; i<N; ++i)
		{
					 pg.line(Vec(x(i,   r1), y(i,   r1)), Vec(x(i, r2), y(i, r2)), "stroke:#440022;stroke-width:1");
			if (i) { pg.line(Vec(x(i-1, r1), y(i-1, r1)), Vec(x(i, r1), y(i, r1)), "stroke:#440022;stroke-width:1"); }
		}
*/
		// Ports
		pg.circle(Vec(x(0, 0), y(0, 0)), 10, "stroke:#440022;stroke-width:1");
		for (std::size_t i=0; i<N; ++i)
		{
			 pg.circle(Vec(x(i, r2), y(i,   r2)), 10, "stroke:#440022;stroke-width:1");
		}

		pg.prt_out(-0.20, 2,          "", "-2");
		pg.prt_out( 0.15, 2,          "", "-1");
		pg.prt_out( 0.50, 2, "TRANSPOSE",  "0");
		pg.prt_out( 0.85, 2,          "", "+1");
		pg.prt_out( 1.20, 2,          "", "+2");
	}
	#endif

	{
		#if GTX__LOAD_SVG
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load("plugins/Gratrix/res/Octave.svg"));
		#else
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load("plugins/Gratrix/res/Octave.png");
		#endif
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	addInput(createInput <PJ301MPort>(prt(x(0, 0), y(0, 0)), module, Octave::IN_INPUT));
	for (std::size_t i=0; i<N; ++i)
	{
		addOutput(createOutput<PJ301MPort>(prt(x(i, r2), y(i, r2)), module, i + Octave::NOTE_OUTPUT));
	}

	addOutput(createOutput<PJ301MPort>(prt(gx(-0.20), gy(2)), module, 0 + Octave::OCT_OUTPUT));
	addOutput(createOutput<PJ301MPort>(prt(gx( 0.15), gy(2)), module, 1 + Octave::OCT_OUTPUT));
	addOutput(createOutput<PJ301MPort>(prt(gx( 0.50), gy(2)), module, 2 + Octave::OCT_OUTPUT));
	addOutput(createOutput<PJ301MPort>(prt(gx( 0.85), gy(2)), module, 3 + Octave::OCT_OUTPUT));
	addOutput(createOutput<PJ301MPort>(prt(gx( 1.20), gy(2)), module, 4 + Octave::OCT_OUTPUT));

	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.5) - 30, fy(0-0.28) + 5), module, Octave::KEY_LIGHT +  0));  // C
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.5) - 25, fy(0-0.28) - 5), module, Octave::KEY_LIGHT +  1));  // C#
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.5) - 20, fy(0-0.28) + 5), module, Octave::KEY_LIGHT +  2));  // D
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.5) - 15, fy(0-0.28) - 5), module, Octave::KEY_LIGHT +  3));  // Eb
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.5) - 10, fy(0-0.28) + 5), module, Octave::KEY_LIGHT +  4));  // E
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.5)     , fy(0-0.28) + 5), module, Octave::KEY_LIGHT +  5));  // F
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.5) +  5, fy(0-0.28) - 5), module, Octave::KEY_LIGHT +  6));  // Fs
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.5) + 10, fy(0-0.28) + 5), module, Octave::KEY_LIGHT +  7));  // G
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.5) + 15, fy(0-0.28) - 5), module, Octave::KEY_LIGHT +  8));  // Ab
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.5) + 20, fy(0-0.28) + 5), module, Octave::KEY_LIGHT +  9));  // A
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.5) + 25, fy(0-0.28) - 5), module, Octave::KEY_LIGHT + 10));  // Bb
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.5) + 30, fy(0-0.28) + 5), module, Octave::KEY_LIGHT + 11));  // B

	for (std::size_t i=0; i<LO_SIZE; ++i)
	{
		addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.5) + (i - LO_SIZE/2) * 10, fy(0-0.28) + 20), module, Octave::OCT_LIGHT + i));
	}
}
