/**
 \file macrotaskq.h
 \brief Declares the \c macrotaskq and MacroTaskBase classes
 \ingroup mra

 A MacroTaskq executes tasks on World objects, e.g. differentiation of a function or other
 arithmetic. Complex algorithms can be implemented.

 The universe world is split into subworlds, each of them executing macrotasks of the task queue.
 This improves locality and speedups for large number of compute nodes, by reducing communications
 within worlds.

 The user defines a macrotask (an example is found in test_vectormacrotask.cc), the tasks are
 lightweight and carry only bookkeeping information, actual input and output are stored in a
 cloud (see cloud.h)

 The user-defined macrotask is derived from MacroTaskIntermediate and must implement the run()
 method. A heterogeneous task queue is possible.

 TODO: priority q
 TODO: task submission from inside task (serialize task instead of replicate)
 TODO: update documentation
 TODO: consider serializing task member variables

*/



#ifndef SRC_MADNESS_MRA_MACROTASKQ_H_
#define SRC_MADNESS_MRA_MACROTASKQ_H_

#include <madness/world/cloud.h>
#include <madness/world/world.h>
#include <madness/mra/macrotaskpartitioner.h>

namespace madness {

/// base class
class MacroTaskBase {
public:

	typedef std::vector<std::shared_ptr<MacroTaskBase> > taskqT;

	MacroTaskBase() {}
	virtual ~MacroTaskBase() {};

	double priority=1.0;
	enum Status {Running, Waiting, Complete, Unknown} stat=Unknown;

	void set_complete() {stat=Complete;}
	void set_running() {stat=Running;}
	void set_waiting() {stat=Waiting;}

	bool is_complete() const {return stat==Complete;}
	bool is_running() const {return stat==Running;}
	bool is_waiting() const {return stat==Waiting;}

	virtual void run(World& world, Cloud& cloud, taskqT& taskq) = 0;
	virtual void cleanup() = 0;		// clear static data (presumably persistent input data)

    virtual void print_me(std::string s="") const {
        printf("this is task with priority %4.1f\n",priority);
    }
    virtual void print_me_as_table(std::string s="") const {
        print("nothing to print");
    }
    std::string print_priority_and_status_to_string() const {
        std::stringstream ss;
        ss << std::setw(5) << this->get_priority() << "  " <<this->stat;
        return ss.str();
    }

    double get_priority() const {return priority;}

    friend std::ostream& operator<<(std::ostream& os, const MacroTaskBase::Status s) {
    	if (s==MacroTaskBase::Status::Running) os << "Running";
    	if (s==MacroTaskBase::Status::Waiting) os << "Waiting";
    	if (s==MacroTaskBase::Status::Complete) os << "Complete";
    	if (s==MacroTaskBase::Status::Unknown) os << "Unknown";
    	return os;
    }
};


template<typename macrotaskT>
class MacroTaskIntermediate : public MacroTaskBase {

public:

	MacroTaskIntermediate() {}

	~MacroTaskIntermediate() {}

	void run(World& world, Cloud& cloud) {
		dynamic_cast<macrotaskT*>(this)->run(world,cloud);
		world.gop.fence();
	}

	void cleanup() {};
};



class MacroTaskQ : public WorldObject< MacroTaskQ> {

    World& universe;
    std::shared_ptr<World> subworld_ptr;
	MacroTaskBase::taskqT taskq;
	std::mutex taskq_mutex;
	long printlevel=0;
	long nsubworld=1;
    std::shared_ptr< WorldDCPmapInterface< Key<1> > > pmap1;
    std::shared_ptr< WorldDCPmapInterface< Key<2> > > pmap2;
    std::shared_ptr< WorldDCPmapInterface< Key<3> > > pmap3;
    std::shared_ptr< WorldDCPmapInterface< Key<4> > > pmap4;
    std::shared_ptr< WorldDCPmapInterface< Key<5> > > pmap5;
    std::shared_ptr< WorldDCPmapInterface< Key<6> > > pmap6;

	bool printdebug() const {return printlevel>=10;}
    bool printtimings() const {return universe.rank()==0 and printlevel>=3;}

public:

	madness::Cloud cloud;
	World& get_subworld() {return *subworld_ptr;}
	long get_nsubworld() const {return nsubworld;}
	void set_printlevel(const long p) {printlevel=p;}

    /// create an empty taskq and initialize the subworlds
	MacroTaskQ(World& universe, int nworld, const long printlevel=0)
		  : universe(universe), WorldObject<MacroTaskQ>(universe), taskq(), cloud(universe), printlevel(printlevel),
		    nsubworld(nworld) {

		subworld_ptr=create_worlds(universe,nworld);
		this->process_pending();
	}

	~MacroTaskQ() {}

	/// for each process create a world using a communicator shared with other processes by round-robin
	/// copy-paste from test_world.cc
	static std::shared_ptr<World> create_worlds(World& universe, const std::size_t nsubworld) {

		int color = universe.rank() % nsubworld;
		SafeMPI::Intracomm comm = universe.mpi.comm().Split(color, universe.rank() / nsubworld);

		std::shared_ptr<World> all_worlds;
		all_worlds.reset(new World(comm));

		universe.gop.fence();
		return all_worlds;
	}

	/// run all tasks, tasks may store the results in the cloud
	void run_all(MacroTaskBase::taskqT vtask=MacroTaskBase::taskqT()) {

		for (const auto& t : vtask) if (universe.rank()==0) t->set_waiting();
		for (int i=0; i<vtask.size(); ++i) add_replicated_task(vtask[i]);
		if (printdebug()) print_taskq();

		cloud.replicate();
        universe.gop.fence();
        if (printdebug()) cloud.print_size(universe);
        universe.gop.set_forbid_fence(true); // make sure there are no hidden universe fences
        pmap1=FunctionDefaults<1>::get_pmap();
        pmap2=FunctionDefaults<2>::get_pmap();
        pmap3=FunctionDefaults<3>::get_pmap();
        pmap4=FunctionDefaults<4>::get_pmap();
        pmap5=FunctionDefaults<5>::get_pmap();
        pmap6=FunctionDefaults<6>::get_pmap();
        set_pmap(get_subworld());

        double cpu00=cpu_time();

		World& subworld=get_subworld();
//		if (printdebug()) print("I am subworld",subworld.id());
		double tasktime=0.0;
		while (true){
			long element=get_scheduled_task_number(subworld);
            double cpu0=cpu_time();
			if (element<0) break;
			std::shared_ptr<MacroTaskBase> task=taskq[element];
            if (printdebug()) print("starting task no",element, "in subworld",subworld.id(),"at time",wall_time());

			task->run(subworld,cloud, taskq);

			double cpu1=cpu_time();
            set_complete(element);
			tasktime+=(cpu1-cpu0);
			if (subworld.rank()==0 and printlevel>=3) printf("completed task %3ld after %6.1fs at time %6.1fs\n",element,cpu1-cpu0,wall_time());

		}
        universe.gop.set_forbid_fence(false);
		universe.gop.fence();
		universe.gop.sum(tasktime);
        double cpu11=cpu_time();
        if (printlevel>=3) cloud.print_timings(universe);
        if (printtimings()) {
            printf("completed taskqueue after    %4.1fs at time %4.1fs\n", cpu11 - cpu00, wall_time());
            printf(" total cpu time / per world  %4.1fs %4.1fs\n", tasktime, tasktime / universe.size());
        }

		// cleanup task-persistent input data
		for (auto& task : taskq) task->cleanup();
		cloud.clear_cache(subworld);
		subworld.gop.fence();
        subworld.gop.fence();
        universe.gop.fence();
        FunctionDefaults<1>::set_pmap(pmap1);
        FunctionDefaults<2>::set_pmap(pmap2);
        FunctionDefaults<3>::set_pmap(pmap3);
        FunctionDefaults<4>::set_pmap(pmap4);
        FunctionDefaults<5>::set_pmap(pmap5);
        FunctionDefaults<6>::set_pmap(pmap6);
        universe.gop.fence();
	}

	void add_tasks(MacroTaskBase::taskqT& vtask) {
        for (const auto& t : vtask) {
            if (universe.rank()==0) t->set_waiting();
            add_replicated_task(t);
        }
	}

    void print_taskq() const {
        universe.gop.fence();
        if (universe.rank()==0) {
            print("\ntaskq on universe rank",universe.rank());
            print("total number of tasks: ",taskq.size());
            print(" task                                   batch                 priority  status");
            for (const auto& t : taskq) t->print_me_as_table();
        }
        universe.gop.fence();
    }

private:
	void add_replicated_task(const std::shared_ptr<MacroTaskBase>& task) {
		taskq.push_back(task);
	}

	/// scheduler is located on universe.rank==0
	long get_scheduled_task_number(World& subworld) {
		long number=0;
		if (subworld.rank()==0) number=this->send(ProcessID(0), &MacroTaskQ::get_scheduled_task_number_local);
		subworld.gop.broadcast_serializable(number, 0);
		subworld.gop.fence();
		return number;

	}

	long get_scheduled_task_number_local() {
		MADNESS_ASSERT(universe.rank()==0);
		std::lock_guard<std::mutex> lock(taskq_mutex);

		auto is_Waiting = [](const std::shared_ptr<MacroTaskBase>& mtb_ptr) {return mtb_ptr->is_waiting();};
		auto it=std::find_if(taskq.begin(),taskq.end(),is_Waiting);
		if (it!=taskq.end()) {
			it->get()->set_running();
			long element=it-taskq.begin();
			return element;
		}
//		print("could not find task to schedule");
		return -1;
	}

	/// scheduler is located on rank==0
	void set_complete(const long task_number) const {
		this->task(ProcessID(0), &MacroTaskQ::set_complete_local, task_number);
	}

	/// scheduler is located on rank==0
	void set_complete_local(const long task_number) const {
		MADNESS_ASSERT(universe.rank()==0);
		taskq[task_number]->set_complete();
	}

public:
	void static set_pmap(World& world) {
        FunctionDefaults<1>::set_default_pmap(world);
        FunctionDefaults<2>::set_default_pmap(world);
        FunctionDefaults<3>::set_default_pmap(world);
        FunctionDefaults<4>::set_default_pmap(world);
        FunctionDefaults<5>::set_default_pmap(world);
        FunctionDefaults<6>::set_default_pmap(world);
	}
private:
	std::size_t size() const {
		return taskq.size();
	}

};




template<typename taskT>
class MacroTask {
    using partitionT = MacroTaskPartitioner::partitionT;

    template<typename Q>
    struct is_vector : std::false_type {
    };
    template<typename Q>
    struct is_vector<std::vector<Q>> : std::true_type {
    };

    typedef typename taskT::resultT resultT;
    typedef typename taskT::argtupleT argtupleT;
    typedef Cloud::recordlistT recordlistT;
    taskT task;

public:

    MacroTask(World &world, taskT &task, std::shared_ptr<MacroTaskQ> taskq_ptr = 0)
            : world(world), task(task), taskq_ptr(taskq_ptr) {
        if (taskq_ptr) {
            // for the time being this condition must hold because tasks are
            // constructed as replicated objects and are not broadcast to other processes
            MADNESS_CHECK(world.id()==taskq_ptr->get_world().id());
        }
    }

    /// this mimicks the original call to the task functor, called from the universe

    /// store all input to the cloud, create output Function<T,NDIM> in the universe,
    /// create the batched task and shove it into the taskq. Possibly execute the taskq.
    template<typename ... Ts>
    resultT operator()(const Ts &... args) {

        const bool immediate_execution = (not taskq_ptr);
        if (not taskq_ptr) taskq_ptr.reset(new MacroTaskQ(world, world.size()));

        auto argtuple = std::tie(args...);
        static_assert(std::is_same<decltype(argtuple), argtupleT>::value, "type or number of arguments incorrect");

        // partition the argument vector into batches
        auto partitioner=task.partitioner;
        if (not partitioner) partitioner.reset(new MacroTaskPartitioner);
        partitioner->set_nsubworld(world.size());
        partitionT partition = partitioner->partition_tasks(argtuple);

        // store input and output: output being a pointer to a universe function (vector)
        recordlistT inputrecords = taskq_ptr->cloud.store(world, argtuple);
        auto[outputrecords, result] =prepare_output(taskq_ptr->cloud, argtuple);

        // create tasks and add them to the taskq
        MacroTaskBase::taskqT vtask;
        for (const auto& batch_prio : partition) {
            vtask.push_back(
                    std::shared_ptr<MacroTaskBase>(new MacroTaskInternal(task, batch_prio, inputrecords, outputrecords)));
        }
        taskq_ptr->add_tasks(vtask);

        if (immediate_execution) taskq_ptr->run_all(vtask);

        return result;
    }

private:

    World &world;
    std::shared_ptr<MacroTaskQ> taskq_ptr;

    /// prepare the output of the macrotask: Function<T,NDIM> must be created in the universe
    std::pair<recordlistT, resultT> prepare_output(Cloud &cloud, const argtupleT &argtuple) {
        static_assert(is_madness_function<resultT>::value || is_madness_function_vector<resultT>::value);
        resultT result = task.allocator(world, argtuple);
        recordlistT outputrecords;
        if constexpr (is_madness_function<resultT>::value) {
            outputrecords += cloud.store(world, result.get_impl().get()); // store pointer to FunctionImpl
        } else if constexpr (is_vector<resultT>::value) {
            outputrecords += cloud.store(world, get_impl(result));
        } else {
            MADNESS_EXCEPTION("\n\n  unknown result type in prepare_input ", 1);
        }
        return std::make_pair(outputrecords, result);
    }

    class MacroTaskInternal : public MacroTaskIntermediate<MacroTask> {

        typedef decay_tuple<typename taskT::argtupleT> argtupleT;   // removes const, &, etc
        typedef typename taskT::resultT resultT;
        recordlistT inputrecords;
        recordlistT outputrecords;
    public:
        taskT task;

        MacroTaskInternal(const taskT &task, const std::pair<Batch,double> &batch_prio,
                          const recordlistT &inputrecords, const recordlistT &outputrecords)
                : task(task), inputrecords(inputrecords), outputrecords(outputrecords) {
            static_assert(is_madness_function<resultT>::value || is_madness_function_vector<resultT>::value);
            this->task.batch=batch_prio.first;
            this->priority=batch_prio.second;
        }


        virtual void print_me(std::string s="") const {
            print("this is task",typeid(task).name(),"with batch", task.batch,"priority",this->get_priority());
        }

        virtual void print_me_as_table(std::string s="") const {
            std::stringstream ss;
            std::string name=typeid(task).name();
            std::size_t namesize=std::min(std::size_t(28),name.size());
            name += std::string(28-namesize,' ');

            std::stringstream ssbatch;
            ssbatch << task.batch;
            std::string strbatch=ssbatch.str();
            int nspaces=std::max(int(0),35-int(ssbatch.str().size()));
            strbatch+=std::string(nspaces,' ');

            ss  << name
                << std::setw(10) << strbatch
                << this->print_priority_and_status_to_string();
            print(ss.str());
        }

        void run(World &subworld, Cloud &cloud, MacroTaskBase::taskqT &taskq) {

            const argtupleT argtuple = cloud.load<argtupleT>(subworld, inputrecords);
            const argtupleT batched_argtuple = task.batch.template copy_input_batch(argtuple);

            resultT result_tmp = std::apply(task, batched_argtuple);

            resultT result = get_output(subworld, cloud, argtuple);       // lives in the universe
            if constexpr (is_madness_function<resultT>::value) {
                result_tmp.compress();
                result += result_tmp;
            } else if constexpr(is_madness_function_vector<resultT>::value) {
                compress(subworld, result_tmp);
                resultT tmp1=task.allocator(subworld,argtuple);
                tmp1=task.batch.template insert_result_batch(tmp1,result_tmp);
                result += tmp1;
            } else {
                MADNESS_EXCEPTION("failing result",1);
            }

        };

        /// get the pointers to the output functions living in the universe

        /// the result of a task living in subworld will be accumulated into result
        /// living in the universe
        resultT get_output(World &subworld, Cloud &cloud, const argtupleT &argtuple) {
            resultT result;
            if constexpr (is_madness_function<resultT>::value) {
                typedef std::shared_ptr<typename resultT::implT> impl_ptrT;
                result.set_impl(cloud.load<impl_ptrT>(subworld, outputrecords));
            } else if constexpr (is_madness_function_vector<resultT>::value) {
                typedef std::shared_ptr<typename resultT::value_type::implT> impl_ptrT;
                std::vector<impl_ptrT> rimpl = cloud.load<std::vector<impl_ptrT>>(subworld, outputrecords);
                result.resize(rimpl.size());
                set_impl(result, rimpl);
            } else {
                MADNESS_EXCEPTION("unknown result type in get_output", 1);
            }
            return result;
        }

    };

};

class MacroTaskOperationBase {
public:
    Batch batch;
    std::shared_ptr<MacroTaskPartitioner> partitioner=0;
    MacroTaskOperationBase() : batch(Batch(_, _, _)), partitioner(new MacroTaskPartitioner) {}
};


} /* namespace madness */

#endif /* SRC_MADNESS_MRA_MACROTASKQ_H_ */
