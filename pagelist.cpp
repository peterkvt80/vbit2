/** PageList
 */
#include "pagelist.h"

using namespace ttx;

PageList::PageList(Configure *configure) :
	_configure(configure)
{
    for (int i=0;i<8;i++)
        _mag[i]=NULL;
	if (_configure==NULL)
	{
		std::cerr << "NULL configuration object" << std::endl;
		return;
	}
	//_filenames={};
	//std::cerr << "[PageList::PageList] Started " << std::endl;
	//std::cerr << "[PageList::PageList] Directory= " << _configure->GetPageDirectory() << std::endl;
	LoadPageList(_configure->GetPageDirectory());
}

PageList::~PageList()
{
	std::cerr << "[PageList] Destructor" << std::endl;
}

int PageList::LoadPageList(std::string filepath)
{
	// std::cerr << "[PageList::LoadPageList] Loading pages from " << filepath << std::endl;
	// Open the directory
	DIR *dp;
	//TTXPageStream *p;
	TTXPageStream* q;
	struct dirent *dirp;
	if ((dp = opendir(filepath.c_str())) == NULL) {
		std::cerr << "Error(" << errno << ") opening " << filepath << std::endl;
		return errno;
	}
	// Load the filenames into a list
	while ((dirp = readdir(dp)) != NULL) {
		//p=new TTXPageStream(filepath+"/"+dirp->d_name);
		q=new TTXPageStream(filepath+"/"+dirp->d_name);
		// If the page loaded, then push it into the appropriate magazine
		if (q->Loaded())
        {
            int mag=(q->GetPageNumber() >> 16) & 0x7;
            _pageList[mag].push_back(*q);
            // std::cerr << "[PageList::LoadPage] Added one page to mag " << mag << "page number=" << q->GetPageNumber()<< std::endl;
            // q->DebugDump();
        }
	}
	closedir(dp);
    std::cerr << "FINISHED LOADING PAGES" << std::endl;

	// How many files did we accept?
	for (int i=0;i<8;i++)
    {
        //std::cerr << "Page list count[" << i << "]=" << _pageList[i].size() << std::endl;
        // Initialise a magazine streamer with a page list
/*
        std::list<TTXPageStream> pageSet;
        pageSet=_pageList[i];
        _mag[i]=new vbit::Mag(pageSet);
*/
        _mag[i]=new vbit::Mag(i, &_pageList[i]);
    }
    // Just for testing
    if (1) for (int i=0;i<8;i++)
    {
        vbit::Mag* m=_mag[i];
        std::list<TTXPageStream>* p=m->Get_pageSet();
        for (std::list<TTXPageStream>::const_iterator it=p->begin();it!=p->end();++it)
        {
            if (&(*it)==NULL)
                std::cerr << "[PageList::LoadPageList] This can't happen unless something is broken" << std::endl;
           // std::cerr << "[PageList::LoadPageList]Dumping :" << std::endl;
           // it->DebugDump();
           std::cerr << "[PageList::LoadPageList] mag["<<i<<"] Filename =" << it->GetSourcePage()  << std::endl;
        }
    }

	return 0;
}
