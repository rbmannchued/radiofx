PREFIX   = ~/.lv2
BUNDLE   = RadioFx.lv2
TARGET   = radioFx.so
SOURCES  = radioFx.c

all:
	mkdir -p $(BUNDLE)
	gcc -fPIC -shared -o $(BUNDLE)/$(TARGET) $(SOURCES) `pkg-config --cflags --libs lv2`
	cp manifest.ttl radioFx.ttl $(BUNDLE)

install: all
	cp -r $(BUNDLE) $(PREFIX)/

clean:
	rm -rf $(BUNDLE)
