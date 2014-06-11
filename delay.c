#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "delay.h"
#include "util.h"

struct delay_state {
	sample_t **bufs;
	ssize_t len, p;
};

void delay_effect_run(struct effect *e, ssize_t *frames, sample_t *ibuf, sample_t *obuf)
{
	int i, k;
	struct delay_state *state = (struct delay_state *) e->data;
	for (i = 0; i < *frames; ++i) {
		for (k = 0; k < e->istream.channels; ++k) {
			if (GET_BIT(e->channel_selector, k) && state->len > 0) {
				obuf[i * e->istream.channels + k] = state->bufs[k][state->p];
				state->bufs[k][state->p] = ibuf[i * e->istream.channels + k];
			}
			else
				obuf[i * e->istream.channels + k] = ibuf[i * e->istream.channels + k];
		}
		state->p = (state->p + 1 >= state->len) ? 0 : state->p + 1;
	}
}

void delay_effect_reset(struct effect *e)
{
	int i;
	struct delay_state *state = (struct delay_state *) e->data;
	for (i = 0; i < e->istream.channels; ++i)
		if (GET_BIT(e->channel_selector, i) && state->len > 0)
			memset(state->bufs[i], 0, state->len * sizeof(sample_t));
	state->p = 0;
}

void delay_effect_drain(struct effect *e, ssize_t *frames, sample_t *obuf)
{
	*frames = 0;
}

void delay_effect_destroy(struct effect *e)
{
	int i;
	struct delay_state *state = (struct delay_state *) e->data;
	for (i = 0; i < e->istream.channels; ++i)
		free(state->bufs[i]);
	free(state->bufs);
	free(state);
}

struct effect * delay_effect_init(struct effect_info *ei, struct stream_info *istream, char *channel_selector, int argc, char **argv)
{
	struct effect *e;
	struct delay_state *state;
	int i;
	double d;

	if (argc != 2) {
		LOG(LL_ERROR, "dsp: %s: usage: %s\n", argv[0], ei->usage);
		return NULL;
	}

	d = atof(argv[1]);
	CHECK_RANGE(d >= 0, "delay");
	state = calloc(1, sizeof(struct delay_state));
	state->len = lround(d * istream->fs);
	state->bufs = calloc(istream->channels, sizeof(sample_t *));
	for (i = 0; i < istream->channels; ++i)
		if (GET_BIT(channel_selector, i) && state->len > 0)
			state->bufs[i] = calloc(state->len, sizeof(sample_t));

	e = calloc(1, sizeof(struct effect));
	e->name = ei->name;
	e->istream.fs = e->ostream.fs = istream->fs;
	e->istream.channels = e->ostream.channels = istream->channels;
	e->channel_selector = NEW_BIT_ARRAY(istream->channels);
	COPY_BIT_ARRAY(e->channel_selector, channel_selector, istream->channels);
	e->ratio = 1.0;
	e->run = delay_effect_run;
	e->reset = delay_effect_reset;
	e->drain = delay_effect_drain;
	e->destroy = delay_effect_destroy;
	e->data = state;
	return e;
}
