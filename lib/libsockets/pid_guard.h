
#ifndef PID_GUARD_H
#define PID_GUARD_H

#include <fstream>
#include <sstream>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>

class CPidGuard
{
public:
	bool IsProcessAlive(const char* pszProcessName) 
	{ 
		assert(pszProcessName);

		std::stringstream oss;
		oss << basename(const_cast<char*>(pszProcessName));
		std::string sThisPidFile = "/var/tmp/";
		sThisPidFile += oss.str();
		sThisPidFile += ".pid";

		std::ifstream ifStream;
		ifStream.open(sThisPidFile.c_str());
		if(!ifStream.good())//�����ɹ�,˵���޴��ļ�(����ǰ�޽������У�
		{
			WriteThisPid(sThisPidFile.c_str());//����ǰpidд���ļ�����ʾ����������
			return false;
		}

		pid_t iPid = 0;
		ifStream >> iPid;
		ifStream.close();

		oss.str("");
		oss << iPid;
		std::string sProcPidFile = "/proc/";
		sProcPidFile += oss.str();
		sProcPidFile += "/status";

		ifStream.open(sProcPidFile.c_str());
		if(!ifStream.good())//�����ɹ�������ǰ���ļ��м�¼��pid��˵���ɽ������˳����������⣺��������ռ���˾ɵ�pid�ͻᷢ���жϴ���
		{
			WriteThisPid(sThisPidFile.c_str());//д���ļ����õ�ǰpid����ɵ�
			return false;
		}
		ifStream.close();

		return true; 
	} 

private:
	void WriteThisPid(const char* pszPidFileName)
	{
		std::ofstream OfStream;
		OfStream.open(pszPidFileName);
		assert(OfStream.good());

		OfStream << getpid();
		OfStream.close();
	}
};

#endif /* PID_GUARD_H */

