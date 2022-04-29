run:
	gnome-terminal -- ./server/server
	python3 client/client.py


clean: 
	rm inpipe outpipe