/* nuklear - v1.00 - public domain */
static int
demo(struct nk_context *ctx)
{
        if (nk_begin(ctx, "Demo", nk_rect(0, 0, 200, 200), NK_WINDOW_SCALABLE))
        {
            enum {EASY, HARD};
            static int op = EASY;
            static int property = 20;

			nk_nxlib_create_window(ctx);
            nk_layout_row_static(ctx, 30, 80, 1);
            if (nk_button_label(ctx, "button"))
                fputs("button pressed\n", stdout);
            nk_layout_row_dynamic(ctx, 30, 2);
            if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
            if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
            nk_layout_row_dynamic(ctx, 25, 1);
            nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);
        }
        nk_end(ctx);
    	return !nk_window_is_closed(ctx, "Demo");
}
