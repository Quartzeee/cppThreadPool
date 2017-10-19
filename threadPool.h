/**
 * ThreadPool class
 * 
 * Summary:
 * This is a simple RAII pool of threads.  It's important to note that this class
 * will only work with void signatures.  If you want to use any other return type this 
 * class must be enhanced or extended (heavily) since this class is unaware of any
 * returns.  It would be a good idea to make this generic one day to process a broader
 * range of function signatures.
 * 
 * Usage:
 * To use this class, declare a ThreadPool and either pass in the number of threads
 * you want to utilize or allow the class to self-determine the number (which will default
 * to the number of hardware threads).  Adding jobs to the class requires you to 
 * bind functions and pass them in as R-values.  This allows the container to have
 * ownership over the call and move any caller resources.
 * To stop the pool, use the kill() method.  This will simply flip our daemon flag
 * and notify all threads to exit.  The caller will have to be careful here since
 * if you kill the threads it doesn't stop the user from adding jobs to the queue.
 * They'll just never be processed it wouldn't be immediately apparent that class
 * has stopped processing (use the bAlive flag to check if need be).
 * 
 */

/* 
 * File:   threadPool.h
 * Author: Jeff Quaintance
 *
 * Created on September 26, 2017, 2:49 PM
 */

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

#ifndef THREADPOOL_H
#define THREADPOOL_H

using namespace std;

class ThreadPool
{
	private:
		bool bAlive;
		mutex _jobsQueueMtx;
		condition_variable cvJobs;
		queue<function<void()>> qJobs;
		vector<thread> vThreads;

		void processJobs();
		void initThreads(const unsigned int &iNumThreads);
	
	public:
		ThreadPool();
		ThreadPool(const unsigned int &iNumThreads);
		~ThreadPool();
		
		void addJob(function<void()> &&newJob);
		void kill();
};

template<typename T>
class tThreadPool
{
	private:
		bool bAlive;
		mutex _jobsQueueMtx;
		condition_variable cvJobs;
		queue<function<T>> qJobs;
		vector<thread> vThreads;

		void processJobs()
		{
			while(this->bAlive)	// Daemon flag
			{	
				function<T> job;	// job storage

				{
					unique_lock<mutex> lck(this->_jobsQueueMtx);	// Obtain a lock on the jobs queue
					this->cvJobs.wait(lck, [&]{ return (!bAlive || !this->qJobs.empty()); });	// Wait for lock, the predicate returns false to continue waiting on spurious wake-ups

					if(!bAlive) return;	// If our daemon has been killed then we are going to exit

					job = move(this->qJobs.front());	// grab the job of our locked queue

					this->qJobs.pop(); // Remove the job from the queue
				} // release lock

				job(); // Process job (if you have a different signature that accepts a param, it would go here.  i.e. void(int) = job(1))
			}
		}
		
		void initThreads(const unsigned int &iNumThreads)
		{
			for(unsigned int i = 0; i <= iNumThreads; i++) 
			{
				this->vThreads.emplace_back(&tThreadPool<T>::processJobs, this);
			}
		}
	
	public:
		tThreadPool() : bAlive(true) { this->initThreads(thread::hardware_concurrency()); };
		tThreadPool(const unsigned int &iNumThreads) : bAlive(true) { this->initThreads(iNumThreads); };
		~tThreadPool()
		{
			this->kill();	// Stop processing
			for(auto &t : this->vThreads) t.join();  // Join all of the threads before destructing
		}
		
		void addJob(function<T> &&tNewJob)
		{
			lock_guard<mutex> lck(this->_jobsQueueMtx); // Obtain a lock on the jobs queue
			this->qJobs.emplace(move(tNewJob));			// Once that lock is obtained, move our function object into the queue
			this->cvJobs.notify_one();					// Notify a waiting thread that work is available
		}
		void kill()
		{
			this->bAlive = false;		// Flip our daemon flag
			this->cvJobs.notify_all();	// Notify all threads to stop waiting and exit
		}
};


#endif /* THREADPOOL_H */

