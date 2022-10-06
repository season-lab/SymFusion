all:
	docker run -ti --cap-add SYS_ADMIN --rm \
		-v `pwd`/../tests/:/symfusion/tests \
		-w /symfusion \
		--name symfusion-run-`date "+%y%m%d-%H%M"` \
		ercoppa/symfusion bash

build:
	docker build -t ercoppa/symfusion -f ./Dockerfile ../

clean:
	docker rmi $(docker images -f "dangling=true" -q)

kill:
	docker kill `docker ps | grep "symfusion-" | awk '{ print $$1 }'`
