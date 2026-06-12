/*
    SPDX-FileCopyrightText: 2026 jannikac <jannikac@mailbox.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    Minimal client for KWin's org_kde_kwin_fake_input protocol, used by the
    automated test to inject pointer motion into a sandboxed KWin instance.

    Usage: fakeinput [abs X Y | rel DX DY COUNT | sleep MS]...
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-client.h>

#include "fake-input-client-protocol.h"

static struct org_kde_kwin_fake_input *s_fake = NULL;

static void registry_global(void *data, struct wl_registry *registry, uint32_t name,
                            const char *interface, uint32_t version)
{
    if (strcmp(interface, org_kde_kwin_fake_input_interface.name) == 0) {
        s_fake = wl_registry_bind(registry, name, &org_kde_kwin_fake_input_interface,
                                  version < 4 ? version : 4);
    }
}

static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
}

static const struct wl_registry_listener s_registryListener = {
    registry_global,
    registry_global_remove,
};

int main(int argc, char **argv)
{
    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "fakeinput: cannot connect to Wayland display\n");
        return 1;
    }
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &s_registryListener, NULL);
    wl_display_roundtrip(display);
    if (!s_fake) {
        fprintf(stderr, "fakeinput: compositor does not advertise org_kde_kwin_fake_input\n");
        return 1;
    }

    org_kde_kwin_fake_input_authenticate(s_fake, "easecursorcrossing-test", "automated plugin test");
    wl_display_roundtrip(display);

    for (int i = 1; i < argc;) {
        if (strcmp(argv[i], "abs") == 0 && i + 2 < argc) {
            org_kde_kwin_fake_input_pointer_motion_absolute(
                s_fake, wl_fixed_from_double(atof(argv[i + 1])), wl_fixed_from_double(atof(argv[i + 2])));
            i += 3;
        } else if (strcmp(argv[i], "rel") == 0 && i + 3 < argc) {
            const double dx = atof(argv[i + 1]);
            const double dy = atof(argv[i + 2]);
            const int count = atoi(argv[i + 3]);
            for (int c = 0; c < count; c++) {
                org_kde_kwin_fake_input_pointer_motion(
                    s_fake, wl_fixed_from_double(dx), wl_fixed_from_double(dy));
                wl_display_roundtrip(display);
                usleep(2000);
            }
            i += 4;
        } else if (strcmp(argv[i], "sleep") == 0 && i + 1 < argc) {
            usleep(atoi(argv[i + 1]) * 1000);
            i += 2;
        } else {
            fprintf(stderr, "fakeinput: bad argument '%s'\n", argv[i]);
            return 1;
        }
        wl_display_roundtrip(display);
    }

    wl_display_disconnect(display);
    return 0;
}
