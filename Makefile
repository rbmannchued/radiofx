PREFIX   = ~/.lv2
BUNDLE   = bandPass.lv2
TARGET   = bandPass.so
SOURCES  = bandPass.c

all:
	mkdir -p $(BUNDLE)
	gcc -fPIC -shared -o $(BUNDLE)/$(TARGET) $(SOURCES) `pkg-config --cflags --libs lv2`
	cp manifest.ttl bandPass.ttl $(BUNDLE)

install: all
	cp -r $(BUNDLE) $(PREFIX)/

clean:
	rm -rf $(BUNDLE)
