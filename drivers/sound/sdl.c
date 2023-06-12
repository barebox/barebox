// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <errno.h>
#include <driver.h>
#include <mach/linux.h>
#include <linux/time.h>
#include <linux/math64.h>
#include <of.h>
#include <sound.h>

#define AMPLITUDE	28000
#define SAMPLERATE	44000ULL

struct sandbox_sound {
	struct sound_card card;
};

static int sandbox_sound_beep(struct sound_card *card, unsigned freq, unsigned duration)
{
    size_t nsamples = div_s64(SAMPLERATE * duration, USEC_PER_SEC);
    int16_t *data;
    int ret;

    if (!freq) {
	    sdl_sound_stop();
	    return 0;
    }

    data = malloc(nsamples * sizeof(*data));
    if (!data)
	    return -ENOMEM;

    synth_sin(freq, AMPLITUDE, data, SAMPLERATE, nsamples);
    ret = sdl_sound_play(data, nsamples);
    if (ret)
	    ret = -EIO;
    free(data);

    return ret;
}

static int sandbox_sound_probe(struct device *dev)
{
	struct sandbox_sound *priv;
	struct sound_card *card;
	int ret;

	priv = xzalloc(sizeof(*priv));

	card = &priv->card;
	card->name = "SDL-Audio";
	card->beep = sandbox_sound_beep;

	ret = sdl_sound_init(SAMPLERATE);
	if (ret) {
		ret = -ENODEV;
		goto free_priv;
	}

	ret = sound_card_register(card);
	if (ret)
		goto sdl_sound_close;

	dev_info(dev, "probed\n");
	return 0;

sdl_sound_close:
	sdl_sound_close();
free_priv:
	free(priv);

	return ret;
}


static __maybe_unused struct of_device_id sandbox_sound_dt_ids[] = {
	{ .compatible = "barebox,sandbox-sound" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sandbox_sound_dt_ids);

static struct driver sandbox_sound_drv = {
	.name  = "sandbox-sound",
	.of_compatible = sandbox_sound_dt_ids,
	.probe = sandbox_sound_probe,
};
device_platform_driver(sandbox_sound_drv);
