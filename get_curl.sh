#!/bin/sh

	
curl -d "{\"address\":\"$1\",\"index\":\"1\",\"command\":\"On\",\"commandData\":\"1\",\"resourceSum\":\"1\"}" 127.0.0.1:8888/zbClient/API/get/state

