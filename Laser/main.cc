#include "lasers.h"
#include "video.h"
#include "oschandler.h"
#include "dbg.h"

void usage(int argc,char *argv[]) {
    fprintf(stderr, "Usage: %s [-P port] [-n nlaser] [-d debug]\n",argv[0]);
    fprintf(stderr,"\t-P port\t\t\tset port to listen on (default: 7780)\n");
    fprintf(stderr,"\t-n nlaser\t\tnumber of laser devices\n");
    fprintf(stderr,"\t-d debug\t\tset debug option (e.g -d4, -dLaser:4)\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    int ch;
    int nlaser=1;
    int port=7780;
    SetDebug("THREAD:1");   // Print thread names in debug messages, if any

    while ((ch=getopt(argc,argv,"d:n:P:"))!=-1) {
	switch (ch) {
	case 'p':
	    port=atoi(optarg);
	    break;
	case 'n':
	    nlaser=atoi(optarg);
	    break;
	case 'd':
	    SetDebug(optarg);
	    break;
	default:
	    usage(argc,argv);
	}
    }
    argc-=optind;
    argv+=optind;
     
    if (argc>0)
	usage(argc,argv);

    
    dbg("main",1) << "Creating lasers" << std::endl;
    std::shared_ptr<Lasers> lasers(new Lasers(nlaser));
    dbg("main",1) << "Creating video" << std::endl;
    std::shared_ptr<Video> video(new Video(lasers));
    dbg("main",1) << "Opening video" << std::endl;
    video->open();
    dbg("main",1) << "Creating OSCHandler on port " << port << std::endl;
    OSCHandler osc(port,lasers,video);

    dbg("main",1) << "Wait forever..." << std::endl;
    osc.wait();
    dbg("main",1) << "Returned from wait forever." << std::endl;
}
