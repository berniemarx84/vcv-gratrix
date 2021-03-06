#include "Gratrix.hpp"
#include "dsp/digital.hpp"

enum Spec
{
	E = 12,    // ET
	T = 25,    // ET
	N = GTX__N
};


// ===========================================================================================================

struct Chord12 : Module
{
	enum ParamIds {
		PROG_PARAM = 0,
		NOTE_PARAM  = PROG_PARAM + 1,
		NUM_PARAMS  = NOTE_PARAM + T
	};
	enum InputIds {
		PROC_INPUT,    // 1
		VOCT_INPUT,    // 1
		NUM_INPUTS,
		OFF_INPUTS = VOCT_INPUT
	};
	enum OutputIds {
		VOCT_OUTPUT,   // N
		NUM_OUTPUTS,
		OFF_OUTPUTS = VOCT_OUTPUT
	};
	enum LightIds {
		PROG_LIGHT = 0,
		NOTE_LIGHT = PROG_LIGHT + E,
		FUND_LIGHT = NOTE_LIGHT + T,
		NUM_LIGHTS = FUND_LIGHT + E
	};

	struct Decode
	{
		float in    = 0;
		float out   = 0;
		int   note  = 0;
		int   key   = 0;
		int   oct   = 0;

		void step(float input)
		{
			int safe, fnote;

			static constexpr float e = static_cast<float>(E);
			static constexpr float s = 1.0f / e;

			in    = input;
			fnote = std::floor(in * e + 0.5f);
			out   = fnote * s;
			note  = static_cast<int>(fnote);
			safe  = note + (E * 1000);  // push away from negative numbers
			key   = safe % E;
			oct   = (safe / E) - 1000;
		}
	};


	Decode config;
	Decode select;
	Decode input;

	SchmittTrigger note_trigger[T];
	bool  note_enable[E][T] = {};
	float gen[N] = {0,1,2,3,4,5};

	Chord12()
	:
		Module(NUM_PARAMS, NUM_INPUTS, N * NUM_OUTPUTS, NUM_LIGHTS)
	{}

	json_t *toJson() override
	{
		json_t *rootJ = json_object();

		// Note enable
		if (json_t *neJA = json_array())
		{
			for (std::size_t i = 0; i < E; ++i)
			{
				for (std::size_t j = 0; j < T; ++j)
				{
					if (json_t *neJI = json_integer((int) note_enable[i][j]))
					{
						json_array_append_new(neJA, neJI);
					}
				}
			}

			json_object_set_new(rootJ, "note_enable", neJA);
		}

		return rootJ;
	}

	void fromJson(json_t *rootJ) override
	{
		// Note enable
		if (json_t *neJA = json_object_get(rootJ, "note_enable"))
		{
			for (std::size_t i = 0; i < E; ++i)
			{
				for (std::size_t j = 0; j < T; ++j)
				{
					if (json_t *neJI = json_array_get(neJA, i*T+j))
					{
						note_enable[i][j] = !!json_integer_value(neJI);
					}
				}
			}
		}
	}

	static constexpr std::size_t omap(std::size_t port, std::size_t bank)
	{
		return port + bank * NUM_OUTPUTS;
	}

	void step() override
	{
		config.step(clampf(params[PROC_INPUT].value, 0.0001, 0.9999) - 0.5f/E);
		select.step(clampf(inputs[PROC_INPUT].normalize(10.0) / 10.0, 0.0, 1.0));
		input .step(inputs[VOCT_INPUT].value);

		{
			for (std::size_t i=0; i<E; ++i)
			{
				lights[PROG_LIGHT + i].value = 0.0f;
				lights[FUND_LIGHT + i].value = 0.0f;
			}

			lights[PROG_LIGHT + select.key].value = +1.0f;
			lights[PROG_LIGHT + config.key].value = -1.0f;
			lights[FUND_LIGHT + input .key].value = +1.0f;
		}

		{
			for (std::size_t j = 0; j < T; ++j)
			{
				if (note_trigger[j].process(params[j + NOTE_PARAM].value))
				{
					note_enable[config.key][j] = !note_enable[config.key][j];
				}
			}

			for (std::size_t j = 0, b = 0; j < T; ++j)
			{
				if (note_enable[config.key][j])
				{
					if (b++ >= N)
					{
						note_enable[config.key][j] = false;
					}
				}
			}

			{
				std::size_t b = 0;

				for (std::size_t j = 0; j < T; ++j)
				{
					if (note_enable[select.key][j])
					{
						lights[NOTE_LIGHT + j].value = +1.0;

						gen[b++] = input.out + static_cast<float>(j) / 12.0f;
					}
					else
					{
						lights[NOTE_LIGHT + j].value = 0.0;
					}

					if (note_enable[config.key][j])
					{
						lights[NOTE_LIGHT + j].value = -1.0;
					}
				}

				while (b < N)
				{
					gen[b++] = -10.0;
				}
			}
		}

		for (std::size_t b=0; b<N; ++b)
		{
			outputs[omap(b, VOCT_OUTPUT)].value = gen[b];
		}
	}
};


int    x (int    i, double radius = 37.0, double spill = 1.65) { return static_cast<int>(6+6*15 + 0.5 + (radius + spill * i) * dx(i, E)); }
int    y (int    i, double radius = 37.0, double spill = 1.65) { return static_cast<int>(180    + 0.5 + (radius + spill * i) * dy(i, E)); }
double xd(double i, double radius = 37.0, double spill = 1.65) { return                 (6+6*15       + (radius + spill * i) * dx(i, E)); }
double yd(double i, double radius = 37.0, double spill = 1.65) { return                 (180          + (radius + spill * i) * dy(i, E)); }


Chord12Widget::Chord12Widget()
{
	GTX__WIDGET();

	Chord12 *module = new Chord12();
	setModule(module);
	box.size = Vec(12*15, 380);

	#if GTX__SAVE_SVG
	{
		PanelGen pg(assetPlugin(plugin, "build/res/Chord12.svg"), box.size, "CHORD");

		pg.nob_med(0.5, -0.28, "PROGRAM SELECT");
		pg.prt_in (0, 2-0.14,  "V/OCT");
		pg.bus_out(1, 2,       "V/OCT");

		for (double i=0.0; i<T-1.0; i+=0.1)
		{
			pg.line(Vec(xd(i), yd(i)), Vec(xd(i+0.1), yd(i+0.1)), "fill:none;stroke:#7092BE;stroke-width:2");
		}

		pg.line(Vec(6+6*15, 180), Vec(xd(24), yd(24)), "fill:none;stroke:#7092BE;stroke-width:4");
		pg.line(Vec(6+6*15, 180), Vec(xd(16), yd(16)), "fill:none;stroke:#7092BE;stroke-width:4");
		pg.line(Vec(6+6*15, 180), Vec(xd(19), yd(19)), "fill:none;stroke:#7092BE;stroke-width:4");
		pg.line(Vec(6+6*15, 180), Vec(xd(22), yd(22)), "fill:none;stroke:#7092BE;stroke-width:4");
	}
	#endif

	{
		#if GTX__LOAD_SVG
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Chord12.svg")));
		#else
		Panel *panel = new LightPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load(assetPlugin(plugin, "res/Chord12.png"));
		#endif
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	addParam(createParam<RoundBlackKnob>(n_m(fx(1+0.18), fy(-0.28)), module, Chord12::PROG_PARAM, 0.0, 1.0, 0.0));

	addInput(createInput<PJ301MPort>(prt(fx(0-0.23), fy(-0.28)),  module, Chord12::PROC_INPUT));
	addInput(createInput<PJ301MPort>(prt(gx(0)     , gy(2+0.14)), module, Chord12::VOCT_INPUT));

	for (std::size_t i=0; i<N; ++i)
	{
		addOutput(createOutput<PJ301MPort>(prt(px(1, i), py(2, i)), module, Chord12::omap(Chord12::VOCT_OUTPUT, i)));
	}

	for (std::size_t i=0; i<T; ++i)
	{
		addParam(createParam<LEDButton>(but(x(i), y(i)), module, i + Chord12::NOTE_PARAM, 0.0, 1.0, 0.0));
		addChild(createLight<SmallLight<GreenRedLight>>(led(x(i), y(i)), module, Chord12::NOTE_LIGHT + i));
	}

	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.45) - 30, fy(0-0.28) + 5), module, Chord12::PROG_LIGHT +  0));  // C
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.45) - 25, fy(0-0.28) - 5), module, Chord12::PROG_LIGHT +  1));  // C#
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.45) - 20, fy(0-0.28) + 5), module, Chord12::PROG_LIGHT +  2));  // D
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.45) - 15, fy(0-0.28) - 5), module, Chord12::PROG_LIGHT +  3));  // Eb
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.45) - 10, fy(0-0.28) + 5), module, Chord12::PROG_LIGHT +  4));  // E
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.45)     , fy(0-0.28) + 5), module, Chord12::PROG_LIGHT +  5));  // F
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.45) +  5, fy(0-0.28) - 5), module, Chord12::PROG_LIGHT +  6));  // Fs
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.45) + 10, fy(0-0.28) + 5), module, Chord12::PROG_LIGHT +  7));  // G
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.45) + 15, fy(0-0.28) - 5), module, Chord12::PROG_LIGHT +  8));  // Ab
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.45) + 20, fy(0-0.28) + 5), module, Chord12::PROG_LIGHT +  9));  // A
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.45) + 25, fy(0-0.28) - 5), module, Chord12::PROG_LIGHT + 10));  // Bb
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.45) + 30, fy(0-0.28) + 5), module, Chord12::PROG_LIGHT + 11));  // B

	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.00) - 30, gy(2-0.14) + 5), module, Chord12::FUND_LIGHT +  0));  // C
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.00) - 25, gy(2-0.14) - 5), module, Chord12::FUND_LIGHT +  1));  // C#
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.00) - 20, gy(2-0.14) + 5), module, Chord12::FUND_LIGHT +  2));  // D
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.00) - 15, gy(2-0.14) - 5), module, Chord12::FUND_LIGHT +  3));  // Eb
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.00) - 10, gy(2-0.14) + 5), module, Chord12::FUND_LIGHT +  4));  // E
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.00)     , gy(2-0.14) + 5), module, Chord12::FUND_LIGHT +  5));  // F
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.00) +  5, gy(2-0.14) - 5), module, Chord12::FUND_LIGHT +  6));  // Fs
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.00) + 10, gy(2-0.14) + 5), module, Chord12::FUND_LIGHT +  7));  // G
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.00) + 15, gy(2-0.14) - 5), module, Chord12::FUND_LIGHT +  8));  // Ab
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.00) + 20, gy(2-0.14) + 5), module, Chord12::FUND_LIGHT +  9));  // A
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.00) + 25, gy(2-0.14) - 5), module, Chord12::FUND_LIGHT + 10));  // Bb
	addChild(createLight<SmallLight<GreenRedLight>>(led(gx(0.00) + 30, gy(2-0.14) + 5), module, Chord12::FUND_LIGHT + 11));  // B
}
