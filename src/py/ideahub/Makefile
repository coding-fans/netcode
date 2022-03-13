# Author: fasion
# Created time: 2022-03-12 14:34:07
# Last Modified by: fasion
# Last Modified time: 2022-03-13 17:00:18

IDEAHUB_VERSION := 1.0
IDEAHUB_IMAGE_URI := fasionchan/ideahub:$(IDEAHUB_VERSION)

ideahub-docker:
	docker build -t $(IDEAHUB_IMAGE_URI) .

push-ideahub-docker: ideahub-docker
	docker push $(IDEAHUB_IMAGE_URI)
