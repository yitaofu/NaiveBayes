
default:	build

clean:
	rm -rf Makefile objs

build:
	$(MAKE) -f objs/Makefile
	$(MAKE) -f objs/Makefile manpage

install:
	$(MAKE) -f objs/Makefile install

upgrade:
	/data/fuyitao/NaiveBayes/4-PredictModelNginx/viw/nginx/sbin/nginx -t

	kill -USR2 `cat /data/fuyitao/NaiveBayes/4-PredictModelNginx/viw/nginx/logs/nginx.pid`
	sleep 1
	test -f /data/fuyitao/NaiveBayes/4-PredictModelNginx/viw/nginx/logs/nginx.pid.oldbin

	kill -QUIT `cat /data/fuyitao/NaiveBayes/4-PredictModelNginx/viw/nginx/logs/nginx.pid.oldbin`
