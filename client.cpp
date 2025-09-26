/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Rahul Saravanakumar
	UIN: 333006421
	Date: 9/25/25
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <iomanip>

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1.0;
	int e = -1;
	int max = MAX_MESSAGE;
	bool new_channel = false;
	
	vector<FIFORequestChannel*> channels;

	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				max = atoi (optarg);
				if (max > MAX_MESSAGE) {
					max = MAX_MESSAGE;
				}
				break;
			case 'c':
				new_channel = true;
				break;
		}
	}

	//give args to server,
	// give args to server,
	// server needs './server', '-m', '<val for m arg>', and 'NULL' to signify end of args
	// fork and in child execvp server

	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	}
	if (pid == 0) {
		// child: exec the server
		char mstr[32];
		snprintf(mstr, sizeof(mstr), "%d", max);
		char *args[] = {(char *)"./server", (char *)"-m", mstr, NULL};
		execvp(args[0], args);
		// if execvp returns, it's an error
		perror("execvp");
		exit(1);
	}

	// parent continues and creates the control channel
	FIFORequestChannel cont_chan("control", FIFORequestChannel::CLIENT_SIDE);

	channels.push_back(&cont_chan);

	if(new_channel){
		// request new channel
		MESSAGE_TYPE m = NEWCHANNEL_MSG;
		cont_chan.cwrite(&m, sizeof(MESSAGE_TYPE));

		char new_channel_name[256];
		cont_chan.cread(new_channel_name, 256);

		FIFORequestChannel* chan = new FIFORequestChannel(new_channel_name, FIFORequestChannel::CLIENT_SIDE);
		channels.push_back(chan);
		
	};

	FIFORequestChannel chan = *(channels.back());



	if(filename != ""){ //we are reading a file, ./client -f 8.csv -m buffer_size

		// sending a non-sense message, you need to change this
		filemsg fm(0, 0);
		string fname = filename;
		
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len]; //request buffer
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan.cwrite(buf2, len);  // I want the file length;

		

		int64_t filesize;
		chan.cread(&filesize, sizeof(int64_t)); //write the file length to the int

		
		//set buffer size to max
		int buffer_size = max;

		//create buffer of size max
		char* buf3 = new char[buffer_size]; //buffer to hold the file


		int number_of_full_chunks = filesize / buffer_size;
		int size_of_last_chunk = filesize % buffer_size;

		if (size_of_last_chunk > 0){
			number_of_full_chunks += 1;
		}
		else{
			size_of_last_chunk = buffer_size;
		}
		
		

		for(int i = 0; i < number_of_full_chunks; i++){

			//request full chunk
			filemsg* file_req = (filemsg*)buf2;
			if (i == number_of_full_chunks-1){
				file_req->length = size_of_last_chunk;
			}
			else{
				file_req->length = buffer_size;	
			}	
			file_req->offset = i * buffer_size;

			chan.cwrite(buf2, len); // send request

			//read full chunk
			chan.cread(buf3, file_req->length); // read file chunk

			//write to file
			ofstream ofs;
			ofs.open(("received/" + fname), ios::out | ios::app | ios::binary);
			ofs.write(buf3, file_req->length);
			ofs.close();

			

		}


		

		delete[] buf2;
		delete[] buf3;


	}

	//check if pte not -1
	else if (p != -1 && t != -1.0 && e != -1) {

		// example data point request
		char buf[MAX_MESSAGE]; // 256
		datamsg x(p, t, e); 

		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // question
		double reply;
		chan.cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
	//if p != -1  and t and e == -1
	else if (p != -1) {
		// ask for first 1000 datapoints for person p
		//spaced by 0.004 seconds
		const int N = 1000;
		double delta = 0.004;

		string outfn = string("received/x1.csv"); //according to the project description
		ofstream ofs(outfn);
		if (!ofs.is_open()) {
			cerr << "Error: could not open output file " << outfn << endl;
		} else {
			char buf[MAX_MESSAGE];
			for (int i = 0; i < N; ++i) {
				double seconds = i * delta;

				// request ecg1
				datamsg d1(p, seconds, 1);
				memcpy(buf, &d1, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg));
				double ecg1;
				chan.cread(&ecg1, sizeof(double));

				// request ecg2
				datamsg d2(p, seconds, 2);
				memcpy(buf, &d2, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg));
				double ecg2;
				chan.cread(&ecg2, sizeof(double));

				// write to CSV: time, ecg1, ecg2
				ofs << seconds << "," << ecg1 << "," << ecg2 << "\n";
			}
			ofs.close();
			cout << "Wrote " << N << " samples to " << outfn << endl;
		}

		
	}

	if(new_channel){
		// closing the new channel
		MESSAGE_TYPE m = QUIT_MSG;
		FIFORequestChannel* c = channels.back();
		c->cwrite(&m, sizeof(MESSAGE_TYPE));
		delete c;
		channels.pop_back();
	}
	
	
    
	
	// closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
}
