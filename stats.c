#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "stats.h"
#include "util.h"

struct stats_state {
	ssize_t samples, peak_count;
	sample_t sum, sum_sq, min, max;
};

void stats_effect_run(struct effect *e, ssize_t *frames, sample_t *ibuf, sample_t *obuf)
{
	ssize_t i, k, samples = *frames * e->ostream.channels;
	struct stats_state *state = (struct stats_state *) e->data;
	for (i = 0; i < samples; i += e->ostream.channels) {
		for (k = 0; k < e->ostream.channels; ++k) {
			++state[k].samples;
			state[k].sum += ibuf[i + k];
			state[k].sum_sq += ibuf[i + k] * ibuf[i + k];
			if (ibuf[i + k] < state[k].min) {
				state[k].min = ibuf[i + k];
				state[k].peak_count = 0;
			}
			if (ibuf[i + k] > state[k].max) {
				state[k].max = ibuf[i + k];
				state[k].peak_count = 0;
			}
			if (ibuf[i + k] == state[k].min || ibuf[i + k] == state[k].max)
				++state[k].peak_count;
			obuf[i + k] = ibuf[i + k];
		}
	}
}

void stats_effect_reset(struct effect *e)
{
	/* do nothing */
}

void stats_effect_drain(struct effect *e, ssize_t *frames, sample_t *obuf)
{
	*frames = -1;
}

void stats_effect_destroy(struct effect *e)
{
	ssize_t i;
	struct stats_state *state = (struct stats_state *) e->data;
	fprintf(stderr, "%-18s", "Channel");
	for (i = 0; i < e->ostream.channels; ++i)
		fprintf(stderr, " %12zd", i);
	fprintf(stderr, "\n%-18s", "DC offset");
	for (i = 0; i < e->ostream.channels; ++i)
		fprintf(stderr, " %12.8f", state[i].sum / state[i].samples);
	fprintf(stderr, "\n%-18s", "Minimum");
	for (i = 0; i < e->ostream.channels; ++i)
		fprintf(stderr, " %12.8f", state[i].min);
	fprintf(stderr, "\n%-18s", "Maximum");
	for (i = 0; i < e->ostream.channels; ++i)
		fprintf(stderr, " %12.8f", state[i].max);
	fprintf(stderr, "\n%-18s", "Peak level (dBFS)");
	for (i = 0; i < e->ostream.channels; ++i)
		fprintf(stderr, " %12.4f", 20 * log10(MAX(fabs(state[i].min), fabs(state[i].max))));
	fprintf(stderr, "\n%-18s", "RMS level (dBFS)");
	for (i = 0; i < e->ostream.channels; ++i)
		fprintf(stderr, " %12.4f", 20 * log10(sqrt(state[i].sum_sq / state[i].samples)));
	fprintf(stderr, "\n%-18s", "Crest factor (dB)");
	for (i = 0; i < e->ostream.channels; ++i)
		fprintf(stderr, " %12.4f", 20 * log10(MAX(fabs(state[i].min), fabs(state[i].max)) / sqrt(state[i].sum_sq / state[i].samples)));
	fprintf(stderr, "\n%-18s", "Peak count");
	for (i = 0; i < e->ostream.channels; ++i)
		fprintf(stderr, " %12zd", state[i].peak_count);
	fprintf(stderr, "\n%-18s", "Samples");
	for (i = 0; i < e->ostream.channels; ++i)
		fprintf(stderr, " %12zd", state[i].samples);
	fprintf(stderr, "\n%-18s", "Length (s)");
	for (i = 0; i < e->ostream.channels; ++i)
		fprintf(stderr, " %12.2f", (double) state[i].samples / e->ostream.fs);
	fputc('\n', stderr);
	free(state);
}

struct effect * stats_effect_init(struct effect_info *ei, struct stream_info *istream, char *channel_selector, int argc, char **argv)
{
	struct effect *e;
	struct stats_state *state;
	e = calloc(1, sizeof(struct effect));
	e->name = ei->name;
	e->istream.fs = e->ostream.fs = istream->fs;
	e->istream.channels = e->ostream.channels = istream->channels;
	e->worst_case_ratio = e->ratio = 1.0;
	e->run = stats_effect_run;
	e->reset = stats_effect_reset;
	e->drain = stats_effect_drain;
	e->destroy = stats_effect_destroy;
	state = calloc(istream->channels, sizeof(struct stats_state));
	e->data = state;
	return e;
}