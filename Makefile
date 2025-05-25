PREFIX   := $(HOME)/.lv2
BUNDLE   := radioFx.lv2
TARGET   := radioFx.so
UI       := radioFx_ui.so
SOURCES  := radioFx.c
UI_SRC   := radioFx_ui.c

LV2_FLAGS  := $(shell pkg-config --cflags --libs lv2 cairo)
GTK_FLAGS  := $(shell pkg-config --cflags --libs gtk+-3.0 lv2)

all: $(BUNDLE)/$(TARGET) $(BUNDLE)/$(UI)
	cp manifest.ttl radioFx.ttl $(BUNDLE)

$(BUNDLE):
	mkdir -p $@

$(BUNDLE)/$(TARGET): $(SOURCES) | $(BUNDLE)
	gcc -fPIC -shared -o $@ $< $(LV2_FLAGS)

$(BUNDLE)/$(UI): $(UI_SRC) | $(BUNDLE)
	gcc -fPIC -shared -o $@ $< \
	-I../libxputty/libxputty/include \
	../libxputty/libxputty/libxputty.a \
	$(LV2_FLAGS) $(GTK_FLAGS)


install: all
	cp -r $(BUNDLE) $(PREFIX)/

clean:
	rm -rf $(BUNDLE)
