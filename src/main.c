#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <stdio.h>
#include <getopt.h>

#include "interp.h"

#define FONT_FILE "fonts/AdwaitaMono-Regular.ttf"

float map_range(float x,
		float in_min, float in_max,
		float out_min, float out_max) {
    if (in_min == in_max)
        return out_min;

    if (x < in_min) x = in_min;
    if (x > in_max) x = in_max;

    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Draws the graph itself and the background lines into a texture
void render_graph(SDL_Renderer *renderer, SDL_Texture **texture,
		  SDL_FPoint *points, SDL_FPoint *point_values,
		  int range_x, int range_y,
		  int window_width, int window_height,
		  Expr_Node *expr) {
	for (int i = 0; i <= window_width; i++) {
		float x1 = i;
		float x2 = map_range(x1, 0, window_width, -range_x, range_x);

		float y1 = eval_expr(expr, x2);
		float y2 = map_range(-y1, -range_y, range_y, 0, window_height - 1);

		// printf("x1 = %f x2 = %f y1 = %f y2 = %f\n", x1, x2, y1, y2);
		points[i] = (SDL_FPoint) { .x = x1, .y = y2 };
		point_values[i] = (SDL_FPoint) { .x = x2, .y = y1 };
	}

	if (*texture)
		SDL_DestroyTexture(*texture);

	*texture = SDL_CreateTexture(renderer,
				SDL_PIXELFORMAT_RGBA32,
				SDL_TEXTUREACCESS_TARGET,
				window_width,
				window_height);
	SDL_SetRenderTarget(renderer, *texture);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);

	// Horizontal graph line
	SDL_RenderLine(renderer,
		       0, window_height / 2,
		       window_width, window_height / 2);

	// Vertical graph line
	SDL_RenderLine(renderer,
		       window_width / 2, 0,
		       window_width / 2, window_height);

	// Horizontal graph mini lines
	for (int i = 0; i <= 2 * (int)range_x + 1; i++) {
		if (i == range_x) continue; // Middle

		int x_coord = map_range(i, 0, range_x * 2, 0, window_width - 1);

		SDL_RenderLine(renderer,
			       x_coord, (window_height / 2) - 16,
			       x_coord, (window_height / 2) + 16);
	}

	// Vertical graph mini lines
	for (int i = 0; i <= 2 * (int)range_y + 1; i++) {
		if (i == range_y) continue; // Middle

		int y_coord = map_range(i, 0, range_y * 2, 0, window_height - 1);

		SDL_RenderLine(renderer,
			       (window_width / 2) - 16, y_coord,
			       (window_width / 2) + 16, y_coord);
	}


	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderLines(renderer, points, window_width + 1);
	SDL_SetRenderTarget(renderer, 0);
}

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("Please provide more arguments!\n");
		return 1;
	}

	float range_x = 10;
	float range_y = 10;
	char *expr = 0;
	int opt = -1;
	while ((opt = getopt(argc, argv, "x:y:")) != -1) {
		switch (opt) {
		case 'x':
			range_x = SDL_atof(optarg);
			break;
		case 'y':
			range_y = SDL_atof(optarg);
			break;
		default:
			return 1;
		}
	}

	if (range_x <= 0) range_x = 5;
	if (range_y <= 0) range_y = 5;

	if (optind < argc) {
		expr = argv[optind];
	} else {
		printf("Please provide an expression to plot!\n");
		return 1;
	}

	Expr_Node *root_node = parse_expression(expr);
	if (!root_node) {
		return 1;
	}

	SDL_Init(SDL_INIT_VIDEO);
	TTF_Init();

	SDL_FPoint points[4096 + 1];
	SDL_FPoint point_values[4096 + 1];
	int highlighted_point = 0;
	int draw_ui = 1;

	int window_width = 800;
	int window_height = 800;
	SDL_Window *window = SDL_CreateWindow("Graph display",
					      window_width, window_height,
					      SDL_WINDOW_RESIZABLE);


	SDL_Renderer *renderer = SDL_CreateRenderer(window, 0);

	SDL_Texture *graph_texture = 0;

	TTF_TextEngine *text_engine = TTF_CreateRendererTextEngine(renderer);
	TTF_Font *font = TTF_OpenFont(FONT_FILE, 22);
	if (!font) {
		printf("%s\n", SDL_GetError());
	}

	render_graph(renderer, &graph_texture, points, point_values,
		     range_x, range_y,
		     window_width, window_height, root_node);

	int running = 1;
	int need_redraw = 1;
	while (running) {
		SDL_Event e;
		if (SDL_WaitEvent(&e)) {
			switch (e.type) {
			case SDL_EVENT_QUIT: {
				running = 0;
				break;
			}
			case SDL_EVENT_KEY_DOWN: {
				SDL_Scancode scancode = e.key.scancode;
				if (scancode == SDL_SCANCODE_ESCAPE ||
				    scancode == SDL_SCANCODE_Q) {
					running = 0;
				} else if (scancode == SDL_SCANCODE_F1) {
					draw_ui = !draw_ui;
					need_redraw = 1;
				}

				if (!draw_ui) break;

				if (scancode == SDL_SCANCODE_LEFT || scancode == SDL_SCANCODE_A) {
					highlighted_point--;
					if (highlighted_point < 0)
						highlighted_point = 0;
					need_redraw = 1;
				} else if (scancode == SDL_SCANCODE_RIGHT || scancode == SDL_SCANCODE_D) {
					highlighted_point++;
					if (highlighted_point >= window_width)
						highlighted_point = window_width;
					need_redraw = 1;
				}

				break;
			}
			case SDL_EVENT_MOUSE_MOTION: {
				if (draw_ui) {
					int m_x = e.motion.x;
					if (highlighted_point == m_x) {

					} else {
						highlighted_point = m_x;
						need_redraw = 1;
					}
				}
				break;
			}
			case SDL_EVENT_WINDOW_RESIZED: {
				window_width = e.window.data1;
				window_height = e.window.data2;
				render_graph(renderer, &graph_texture, points, point_values,
						range_x, range_y,
						window_width, window_height, root_node);
				break;
			}
			}
		}

		if (need_redraw) {
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer);

			SDL_FRect dest = { 0, 0, window_width, window_height };
			SDL_RenderTexture(renderer, graph_texture, 0, &dest);

			if (draw_ui) {
				SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

				int cursor_x = highlighted_point;
				if (cursor_x >= window_width)
					cursor_x -= 1;

				SDL_RenderLine(renderer,
					       cursor_x, 0,
					       cursor_x, window_height);
				SDL_RenderLine(renderer,
					       0, points[highlighted_point].y,
					       window_width, points[highlighted_point].y);

				int anchor_right = highlighted_point <= (window_width / 2);
				char text[512];
				SDL_snprintf(text, 512, "x = % 6.8f\ny = % 6.8f",
					     point_values[highlighted_point].x,
					     point_values[highlighted_point].y);
				int text_len = SDL_strlen(text);
				int text_w, text_h;
				TTF_GetStringSizeWrapped(font, text, text_len, 0, &text_w, &text_h);
				TTF_Text *ttf_text = TTF_CreateText(text_engine, font, text, text_len);
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
				if (anchor_right) {
					SDL_FRect rect = { window_width - text_w, 0, text_w, text_h};
					SDL_RenderFillRect(renderer, &rect);
					TTF_DrawRendererText(ttf_text, window_width - text_w, 0);
				} else {
					SDL_FRect rect = { 0, 0, text_w, text_h};
					SDL_RenderFillRect(renderer, &rect);
					TTF_DrawRendererText(ttf_text, 0, 0);
				}
				TTF_DestroyText(ttf_text);
			}

			SDL_RenderPresent(renderer);

			need_redraw = 0;
		}
	}

	SDL_DestroyTexture(graph_texture);
	TTF_CloseFont(font);
	TTF_DestroyRendererTextEngine(text_engine);
	TTF_Quit();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
