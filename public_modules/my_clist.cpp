//#include "stdafx.h"
#include "stdafx.h"
#include "my_clist.h"


namespace MY_LIST
{

	CPtrList::CPtrList()
	{
#ifndef _WIN32

		pthread_mutexattr_t mutexattr;
		pthread_mutexattr_init(&mutexattr);
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
		pthread_mutex_init(&m_mutex, &mutexattr);
		pthread_mutexattr_destroy(&mutexattr);

#else
		InitializeCriticalSection(&m_mutex);

#endif

		m_ptr = NULL;
		m_maxListCount = 1024;
		m_ptr = new void*[m_maxListCount];
		bNeedCheck = false;
		m_nCount = 0;
		startpos = 0;
	}

	CPtrList::~CPtrList()
	{

		if (m_ptr != NULL)
		{
			delete[]m_ptr;
			m_ptr = NULL;
		}
#ifdef _WIN32

		DeleteCriticalSection(&m_mutex);
#endif

#ifdef __unix__

		pthread_mutex_destroy(&m_mutex);
#endif
	}
	void*	CPtrList::CheckDataExist(void *ptr)
	{
		if (ptr == NULL) return NULL;

		for (int ii = 0; ii < m_nCount; ii++)
		{
                        //printf(" %x %x ",m_ptr[ii],ptr);
			if (m_ptr[ii] == ptr)
			{
				return m_ptr[ii];
			}
		}

		return NULL;
	}
	unsigned int CPtrList::GetMaxCount()
	{

		return m_maxListCount;
	}
	void	CPtrList::lock()
	{
#ifdef _WIN32
		EnterCriticalSection(&m_mutex);
#endif

#ifdef __unix__
		pthread_mutex_lock(&m_mutex);
#endif
	}
	void	CPtrList::unlock()
	{
#ifdef _WIN32
		LeaveCriticalSection(&m_mutex);
#endif

#ifdef __unix__
		pthread_mutex_unlock(&m_mutex);
#endif
	}
	bool CPtrList::ShowAddress()
	{
                printf(" int ShowAddress  nCount= %d  ", m_nCount);
//		for (int ii = 0; ii < m_nCount; ii++)
//		{
//			//  char *p=(char*)m_ptr[(startpos+ii)%m_maxListCount];
//			my_printf(" pos:%d %x", (startpos + ii) % m_maxListCount, (int)m_ptr[(startpos + ii) % m_maxListCount]);

//			if ((ii + 1) % 3 == 0)
//			{
//				my_printf("");
//			}

//		}
//		my_printf(" ");

		return true;
	}
	bool CPtrList::SetCheck(bool check)
	{
		bNeedCheck = check;
		return true;
	}

	bool CPtrList::addMaxListCount(unsigned int addListCount)
	{
		if (addListCount < 1) return false;

		//     unsigned long long ttCount=0;
		//    ttCount=(unsigned long long)(m_maxListCount)+ (unsigned long long)(addListCount);

		if (((unsigned long long)(m_maxListCount) + (unsigned long long)(addListCount)) > 0xffffffff)
		{
			addListCount = 0xffffffff - m_maxListCount;
		}
		void **p = new void*[m_maxListCount + addListCount];
		if (p == NULL) return false;

		lock();

		//printf(" %d %d %d %d\n",startpos,m_nCount,m_maxListCount,m_maxListCount-startpos);
		if (m_ptr != NULL)
		{
			if ((startpos + m_nCount) <= m_maxListCount)
			{
				memcpy(p, m_ptr + startpos, ((sizeof(void *))*(m_nCount)));
				startpos = 0;
			}
			else
			{
				memcpy(p, m_ptr + startpos, ((sizeof(void *))*(m_maxListCount - startpos)));
				memcpy(p + (m_maxListCount - startpos), m_ptr, ((sizeof(void *))*(m_nCount - (m_maxListCount - startpos))));
				startpos = 0;
			}
			delete m_ptr;
		}
		m_ptr = p;
		m_maxListCount += addListCount;
		unlock();
		return true;
	}


	bool CPtrList::AddTail(void* ptr)
	{
		bool r = false;

		lock();
		if (m_nCount < m_maxListCount)
		{
			m_ptr[(startpos + m_nCount) % m_maxListCount] = ptr;
			m_nCount++;
			r = true;
		}

		Check();
		unlock();

		return r;
	}

	bool CPtrList::AddHead(void* ptr)
	{
		bool r = false;

		lock();

		if (m_nCount < m_maxListCount)
		{

			startpos = (startpos == 0) ? (m_maxListCount - 1) : (startpos - 1);
			m_ptr[startpos] = ptr;
			m_nCount++;
			r = true;
		}

		Check();
		unlock();

		return r;
	}

	void* CPtrList::RemoveHead()
	{
		lock();

		void* p = NULL;
		if (m_nCount > 0)
		{

			p = m_ptr[startpos];
			m_ptr[startpos] = 0;
			startpos = (startpos + 1) % m_maxListCount;
			m_nCount--;
		}
		Check();
		unlock();
		return p;
	}

	void* CPtrList::RemoveTail()
	{
		lock();//(&m_mutex);

		void* p = NULL;
		if (m_nCount > 0)
		{
			p = m_ptr[(startpos + m_nCount - 1) % m_maxListCount];
			m_ptr[(startpos + m_nCount - 1) % m_maxListCount] = 0;
			m_nCount--;
		}
		Check();
		unlock();
		return p;
	}

	int CPtrList::GetCount()
	{
		return m_nCount;
	}

	void* CPtrList::GetAt(int index)
	{
		if (index > m_nCount - 1) return NULL;
		if (index < 0) return NULL;
		void* p = NULL;
		// pthread_mutex_lock(&m_mutex);
		lock();
		p = m_ptr[(startpos + index) % m_maxListCount];
		Check();
		unlock();
		//pthread_mutex_unlock(&m_mutex);
		return p;
	}

	void* CPtrList::RemoveAt(int index)
	{
		if (index > m_nCount - 1) return NULL;
		if (index < 0) return NULL;

		void* p = NULL;
		//pthread_mutex_lock(&m_mutex);
		lock();
		p = m_ptr[(startpos + index) % m_maxListCount];

		if (index > m_nCount / 2)
		{
			for (int i = index; i < m_nCount - 1; i++)
			{
				m_ptr[(startpos + i) % m_maxListCount] = m_ptr[(startpos + i + 1) % m_maxListCount];
			}
			m_ptr[(startpos + m_nCount - 1) % m_maxListCount] = 0;
		}
		else
		{

			for (int i = index; i > 0; i--)
			{
				m_ptr[((startpos + i)) % m_maxListCount] = m_ptr[((startpos + i - 1)) % m_maxListCount];
			}
			m_ptr[startpos] = 0;
			startpos = (startpos + 1) % m_maxListCount;
		}
		/**/
		m_nCount--;
		Check();
		// pthread_mutex_unlock(&m_mutex);
		unlock();
		return p;
	}

	bool CPtrList::InsertAt(void* ptr, int index)
	{
		if (m_nCount >= m_maxListCount) return false;
		if (index > m_nCount) return false;
		if (index < 0) return false;

		lock();
		if (index >= m_nCount / 2)
		{
			for (int i = m_nCount; i > index; i--)
			{
				m_ptr[(startpos + i) % m_maxListCount] = m_ptr[(startpos + i - 1) % m_maxListCount];
			}
			m_ptr[(startpos + index) % m_maxListCount] = ptr;

		}
		else
		{

			startpos = (startpos == 0) ? (m_maxListCount - 1) : (startpos - 1);
			for (int i = 0; i < index; i++)
			{
				m_ptr[(startpos + i) % m_maxListCount] = m_ptr[(startpos + i + 1) % m_maxListCount];
			}
			m_ptr[(startpos + index) % m_maxListCount] = ptr;
		}
		m_nCount++;
		Check();
		unlock();
		return true;
	}
	void* CPtrList::GetNext(void* ptr)
	{
		void* p = NULL;
		lock();

		if (ptr == NULL)
		{
			if (m_nCount > 0) p = m_ptr[startpos];
			else p = NULL;
		}
		else
		{
			for (int i = 0; i < m_nCount - 1; i++)
			{
				if (m_ptr[(startpos + i) % m_maxListCount] == ptr)
				{
					p = m_ptr[(startpos + i + 1) % m_maxListCount];
					break;	//added newly
				}
			}
		}

		Check();

		unlock();

		return p;
	}

	bool CPtrList::Remove(void* ptr)
	{
		bool bOK = false;

		lock();

		if (ptr != NULL)
		{
			for (int i = 0; i < m_nCount; i++)
			{
				if (m_ptr[(startpos + i) % m_maxListCount] == ptr)
				{

					if (i >= m_nCount / 2)
					{
						for (int k = i; k < m_nCount - 1; k++)
							m_ptr[(startpos + k) % m_maxListCount] = m_ptr[(startpos + k + 1) % m_maxListCount];
						m_ptr[(startpos + m_nCount - 1) % m_maxListCount] = 0;
					}
					else
					{
						for (int kk = i; kk > 0; kk--)
						{
							m_ptr[((startpos + kk)) % m_maxListCount] = m_ptr[((startpos + kk - 1)) % m_maxListCount];
						}

						m_ptr[startpos] = NULL;
						startpos = (startpos + 1) % m_maxListCount;
					}


					m_nCount--;

					bOK = true;
					break;
				}
			}
		}

		Check();

		unlock();

		return bOK;
	}


	bool CPtrList::Check()

	{
		if (!bNeedCheck)	return true;
		for (int i = 0; i < m_nCount; i++)
		{
			for (int j = i + 1; j < m_nCount; j++)
			{
//				my_printf("%x ", (int)m_ptr[(startpos + j) % m_maxListCount]);

				if (m_ptr[(startpos + i) % m_maxListCount] == m_ptr[(startpos + j) % m_maxListCount])
				{

                                        //my_printf("Error !!!!!!!!!!!!!!!!!!!!!!!!!!!");
					for (int k = 0; k < m_nCount; k++)
					{
                                                //my_printf("%x ", (int)m_ptr[(startpos + k) % m_maxListCount]);
					}
                                        //my_printf("\nError !!!!!!!!!!!!!!!!!!!!!!!!!!!");
					fflush(stdout);

				}
			}
		}

		return true;
	}



    //**************************************list ref***************************



//        CPtrList_ref::CPtrList_ref()
//        {
//#ifndef _WIN32

//                pthread_mutexattr_t mutexattr;
//                pthread_mutexattr_init(&mutexattr);
//                pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
//                pthread_mutex_init(&m_mutex, &mutexattr);
//                pthread_mutexattr_destroy(&mutexattr);

//#else
//                InitializeCriticalSection(&m_mutex);

//#endif

//                m_ptr = NULL;
//                m_maxListCount = 1024;
//                m_ptr = new void*[m_maxListCount];
//                bNeedCheck = false;
//                m_nCount = 0;
//                startpos = 0;
//        }

//        CPtrList_ref::~CPtrList_ref()
//        {

//                if (m_ptr != NULL)
//                {
//                        delete[]m_ptr;
//                        m_ptr = NULL;
//                }
//#ifdef _WIN32

//                DeleteCriticalSection(&m_mutex);
//#endif

//#ifdef __unix__

//                pthread_mutex_destroy(&m_mutex);
//#endif
//        }
//        void*	CPtrList_ref::CheckDataExist(void *ptr)
//        {
//                if (ptr == NULL) return NULL;

//                for (int ii = 0; ii < m_nCount; ii++)
//                {
//                        //printf(" %x %x ",m_ptr[ii],ptr);
//                        if (m_ptr[ii] == ptr)
//                        {
//                                return m_ptr[ii];
//                        }
//                }

//                return NULL;
//        }
//        unsigned int CPtrList_ref::GetMaxCount()
//        {

//                return m_maxListCount;
//        }
//        void	CPtrList_ref::lock()
//        {
//#ifdef _WIN32
//                EnterCriticalSection(&m_mutex);
//#endif

//#ifdef __unix__
//                pthread_mutex_lock(&m_mutex);
//#endif
//        }
//        void	CPtrList_ref::unlock()
//        {
//#ifdef _WIN32
//                LeaveCriticalSection(&m_mutex);
//#endif

//#ifdef __unix__
//                pthread_mutex_unlock(&m_mutex);
//#endif
//        }
//        bool CPtrList_ref::ShowAddress()
//        {
//                printf(" int ShowAddress  nCount= %d  ", m_nCount);
////		for (int ii = 0; ii < m_nCount; ii++)
////		{
////			//  char *p=(char*)m_ptr[(startpos+ii)%m_maxListCount];
////			my_printf(" pos:%d %x", (startpos + ii) % m_maxListCount, (int)m_ptr[(startpos + ii) % m_maxListCount]);

////			if ((ii + 1) % 3 == 0)
////			{
////				my_printf("");
////			}

////		}
////		my_printf(" ");

//                return true;
//        }
//        bool CPtrList_ref::SetCheck(bool check)
//        {
//                bNeedCheck = check;
//                return true;
//        }

//        bool CPtrList_ref::addMaxListCount(unsigned int addListCount)
//        {
//                if (addListCount < 1) return false;

//                //     unsigned long long ttCount=0;
//                //    ttCount=(unsigned long long)(m_maxListCount)+ (unsigned long long)(addListCount);

//                if (((unsigned long long)(m_maxListCount) + (unsigned long long)(addListCount)) > 0xffffffff)
//                {
//                        addListCount = 0xffffffff - m_maxListCount;
//                }
//                void **p = new void*[m_maxListCount + addListCount];
//                if (p == NULL) return false;

//                lock();

//                //printf(" %d %d %d %d\n",startpos,m_nCount,m_maxListCount,m_maxListCount-startpos);
//                if (m_ptr != NULL)
//                {
//                        if ((startpos + m_nCount) <= m_maxListCount)
//                        {
//                                memcpy(p, m_ptr + startpos, ((sizeof(void *))*(m_nCount)));
//                                startpos = 0;
//                        }
//                        else
//                        {
//                                memcpy(p, m_ptr + startpos, ((sizeof(void *))*(m_maxListCount - startpos)));
//                                memcpy(p + (m_maxListCount - startpos), m_ptr, ((sizeof(void *))*(m_nCount - (m_maxListCount - startpos))));
//                                startpos = 0;
//                        }
//                        delete m_ptr;
//                }
//                m_ptr = p;
//                m_maxListCount += addListCount;
//                unlock();
//                return true;
//        }


//        bool CPtrList_ref::AddTail(void* ptr)
//        {
//                bool r = false;

//                lock();
//                if (m_nCount < m_maxListCount)
//                {
//                        m_ptr[(startpos + m_nCount) % m_maxListCount] = ptr;
//                        m_nCount++;
//                        r = true;
//                }

//                Check();
//                unlock();

//                return r;
//        }

//        bool CPtrList_ref::AddHead(void* ptr)
//        {
//                bool r = false;

//                lock();

//                if (m_nCount < m_maxListCount)
//                {

//                        startpos = (startpos == 0) ? (m_maxListCount - 1) : (startpos - 1);
//                        m_ptr[startpos] = ptr;
//                        m_nCount++;
//                        r = true;
//                }

//                Check();
//                unlock();

//                return r;
//        }

//        void* CPtrList_ref::RemoveHead()
//        {
//                lock();

//                void* p = NULL;
//                if (m_nCount > 0)
//                {

//                        p = m_ptr[startpos];
//                        m_ptr[startpos] = 0;
//                        startpos = (startpos + 1) % m_maxListCount;
//                        m_nCount--;
//                }
//                Check();
//                unlock();
//                return p;
//        }

//        void* CPtrList_ref::RemoveTail()
//        {
//                lock();//(&m_mutex);

//                void* p = NULL;
//                if (m_nCount > 0)
//                {
//                        p = m_ptr[(startpos + m_nCount - 1) % m_maxListCount];
//                        m_ptr[(startpos + m_nCount - 1) % m_maxListCount] = 0;
//                        m_nCount--;
//                }
//                Check();
//                unlock();
//                return p;
//        }

//        int CPtrList_ref::GetCount()
//        {
//                return m_nCount;
//        }

//        void* CPtrList_ref::GetAt(int index)
//        {
//                if (index > m_nCount - 1) return NULL;
//                if (index < 0) return NULL;
//                void* p = NULL;
//                // pthread_mutex_lock(&m_mutex);
//                lock();
//                p = m_ptr[(startpos + index) % m_maxListCount];
//                Check();
//                unlock();
//                //pthread_mutex_unlock(&m_mutex);
//                return p;
//        }

//        void* CPtrList_ref::RemoveAt(int index)
//        {
//                if (index > m_nCount - 1) return NULL;
//                if (index < 0) return NULL;

//                void* p = NULL;
//                //pthread_mutex_lock(&m_mutex);
//                lock();
//                p = m_ptr[(startpos + index) % m_maxListCount];

//                if (index > m_nCount / 2)
//                {
//                        for (int i = index; i < m_nCount - 1; i++)
//                        {
//                                m_ptr[(startpos + i) % m_maxListCount] = m_ptr[(startpos + i + 1) % m_maxListCount];
//                        }
//                        m_ptr[(startpos + m_nCount - 1) % m_maxListCount] = 0;
//                }
//                else
//                {

//                        for (int i = index; i > 0; i--)
//                        {
//                                m_ptr[((startpos + i)) % m_maxListCount] = m_ptr[((startpos + i - 1)) % m_maxListCount];
//                        }
//                        m_ptr[startpos] = 0;
//                        startpos = (startpos + 1) % m_maxListCount;
//                }
//                /**/
//                m_nCount--;
//                Check();
//                // pthread_mutex_unlock(&m_mutex);
//                unlock();
//                return p;
//        }

//        bool CPtrList_ref::InsertAt(void* ptr, int index)
//        {
//                if (m_nCount >= m_maxListCount) return false;
//                if (index > m_nCount) return false;
//                if (index < 0) return false;

//                lock();
//                if (index >= m_nCount / 2)
//                {
//                        for (int i = m_nCount; i > index; i--)
//                        {
//                                m_ptr[(startpos + i) % m_maxListCount] = m_ptr[(startpos + i - 1) % m_maxListCount];
//                        }
//                        m_ptr[(startpos + index) % m_maxListCount] = ptr;

//                }
//                else
//                {

//                        startpos = (startpos == 0) ? (m_maxListCount - 1) : (startpos - 1);
//                        for (int i = 0; i < index; i++)
//                        {
//                                m_ptr[(startpos + i) % m_maxListCount] = m_ptr[(startpos + i + 1) % m_maxListCount];
//                        }
//                        m_ptr[(startpos + index) % m_maxListCount] = ptr;
//                }
//                m_nCount++;
//                Check();
//                unlock();
//                return true;
//        }
//        void* CPtrList_ref::GetNext(void* ptr)
//        {
//                void* p = NULL;
//                lock();

//                if (ptr == NULL)
//                {
//                        if (m_nCount > 0) p = m_ptr[startpos];
//                        else p = NULL;
//                }
//                else
//                {
//                        for (int i = 0; i < m_nCount - 1; i++)
//                        {
//                                if (m_ptr[(startpos + i) % m_maxListCount] == ptr)
//                                {
//                                        p = m_ptr[(startpos + i + 1) % m_maxListCount];
//                                }
//                        }
//                }

//                Check();

//                unlock();

//                return p;
//        }

//        bool CPtrList_ref::Remove(void* ptr)
//        {
//                bool bOK = false;

//                lock();

//                if (ptr != NULL)
//                {
//                        for (int i = 0; i < m_nCount; i++)
//                        {
//                                if (m_ptr[(startpos + i) % m_maxListCount] == ptr)
//                                {

//                                        if (i >= m_nCount / 2)
//                                        {
//                                                for (int k = i; k < m_nCount - 1; k++)
//                                                        m_ptr[(startpos + k) % m_maxListCount] = m_ptr[(startpos + k + 1) % m_maxListCount];
//                                                m_ptr[(startpos + m_nCount - 1) % m_maxListCount] = 0;
//                                        }
//                                        else
//                                        {
//                                                for (int kk = i; kk > 0; kk--)
//                                                {
//                                                        m_ptr[((startpos + kk)) % m_maxListCount] = m_ptr[((startpos + kk - 1)) % m_maxListCount];
//                                                }

//                                                m_ptr[startpos] = NULL;
//                                                startpos = (startpos + 1) % m_maxListCount;
//                                        }


//                                        m_nCount--;

//                                        bOK = true;
//                                        break;
//                                }
//                        }
//                }

//                Check();

//                unlock();

//                return bOK;
//        }


//        bool CPtrList_ref::Check()
//        {
//                if (!bNeedCheck)	return true;
//                for (int i = 0; i < m_nCount; i++)
//                {
//                        for (int j = i + 1; j < m_nCount; j++)
//                        {
////				my_printf("%x ", (int)m_ptr[(startpos + j) % m_maxListCount]);

//                                if (m_ptr[(startpos + i) % m_maxListCount] == m_ptr[(startpos + j) % m_maxListCount])
//                                {

//                                        //my_printf("Error !!!!!!!!!!!!!!!!!!!!!!!!!!!");
//                                        for (int k = 0; k < m_nCount; k++)
//                                        {
//                                                //my_printf("%x ", (int)m_ptr[(startpos + k) % m_maxListCount]);
//                                        }
//                                        //my_printf("\nError !!!!!!!!!!!!!!!!!!!!!!!!!!!");
//                                        fflush(stdout);



//                                }
//                        }
//                }

//                return true;
//        }


}


