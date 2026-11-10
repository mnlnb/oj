.PHONY: all output clean

all:
	$(MAKE) -C compile_server
	$(MAKE) -C oj_server

output: all
	mkdir -p output/compile_server/temp output/oj_server
	cp compile_server/compile_server output/compile_server/
	cp -R oj_server/conf oj_server/questions oj_server/template_html oj_server/wwwroot output/oj_server/
	cp oj_server/oj_server output/oj_server/

clean:
	$(MAKE) -C compile_server clean
	$(MAKE) -C oj_server clean
	rm -rf output
