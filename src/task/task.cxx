/* Copyright (c) 2013, Devin Matthews
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following
 * conditions are met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL DEVIN MATTHEWS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE. */

#include "task.hpp"

#include <set>
#include <exception>

using namespace std;
using namespace aquarius;
using namespace aquarius::time;
using namespace aquarius::input;

namespace aquarius
{
namespace task
{

Logger::NullStream Logger::nullstream;
Logger::SelfDestructStream Logger::sddstream;

int Logger::LogToStreamBuffer::sync()
{
    if (os.flush()) return 0;
    else return -1;
}

std::streamsize Logger::LogToStreamBuffer::xsputn (const char* s, std::streamsize n)
{
    if (os.write(s, n)) return n;
    else return 0;
}

int Logger::LogToStreamBuffer::overflow (int c)
{
    if (c != EOF) os << traits_type::to_char_type(c);
    if (c == EOF || !os) return EOF;
    else return traits_type::to_int_type(c);
}

Logger::LogToStreamBuffer::LogToStreamBuffer(std::ostream& os)
: os(os) {}

int Logger::SelfDestructBuffer::sync()
{
    LogToStreamBuffer::sync();
    abort();
    return -1;
}

Logger::SelfDestructBuffer::SelfDestructBuffer()
: LogToStreamBuffer(std::cerr) {}

std::streamsize Logger::NullBuffer::xsputn (const char* s, std::streamsize n)
{
    return n;
}

int Logger::NullBuffer::overflow (int c)
{
    if (c == EOF) return EOF;
    else return traits_type::to_int_type(c);
}

Logger::NullStream::NullStream()
: std::ostream(new NullBuffer()) {}

Logger::NullStream::~NullStream()
{
    delete rdbuf();
}

Logger::SelfDestructStream::SelfDestructStream()
: std::ostream(new SelfDestructBuffer()) {}

std::string Logger::dateTime()
{
    char buf[256];
    time_t t = ::time(NULL);
    tm* timeptr = localtime(&t);
    strftime(buf, 256, "%c", timeptr);
    return std::string(buf);
}

std::ostream& Logger::log(const Arena& arena)
{
    if (arena.rank == 0)
    {
        std::cout << dateTime() << ": ";
        return std::cout;
    }
    else
    {
        return nullstream;
    }
}

std::ostream& Logger::warn(const Arena& arena)
{
    if (arena.rank == 0)
    {
        std::cerr << dateTime() << ": warning: ";
        return std::cerr;
    }
    else
    {
        return nullstream;
    }
}

std::ostream& Logger::error(const Arena& arena)
{
    if (arena.rank == 0)
    {
        //sddstream << dateTime() << ": error: ";
        //return sddstream;
        std::cout << dateTime() << ": error: ";
        return std::cout;
    }
    else
    {
        //pause();
        return nullstream;
    }
}

Requirement::Requirement(const string& type, const string& name)
: type(type), name(name) {}

void Requirement::fulfil(const Product& product)
{
    this->product.reset(new Product(product));
    *this->product->used = true;
}

bool Requirement::exists() const
{
    return product->exists();
}

Product& Requirement::get()
{
    if (!product) throw logic_error("Requirement " + name + " not fulfilled");
    return *product;
}

Product::Product(const string& type, const string& name)
: type(type), name(name), requirements(new vector<Requirement>()), used(new bool(false)) {}

Product::Product(const string& type, const string& name, const vector<Requirement>& reqs)
: type(type), name(name), requirements(new vector<Requirement>(reqs)), used(new bool(false)) {}

void Product::addRequirement(const Requirement& req)
{
    requirements->push_back(req);
}

void Product::addRequirements(const std::vector<Requirement>& reqs)
{
    requirements->insert(requirements->end(), reqs.begin(), reqs.end());
}

template <> string Task::type_string<float>() { return "<float>"; }
template <> string Task::type_string<double>() { return ""; }
template <> string Task::type_string<complex<float> >() { return "<scomplex>"; }
template <> string Task::type_string<complex<double> >() { return "<dcomplex>"; }

Task::Task(const string& type, const string& name)
: type(type), name(name) {}

map<string,Task::factory_func>& Task::tasks()
{
    static std::map<std::string,factory_func> tasks_;
    return tasks_;
}

ostream& Task::log(const Arena& arena)
{
    return Logger::log(arena) << name << ": ";
}

ostream& Task::warn(const Arena& arena)
{
    return Logger::warn(arena) << name << ": ";
}

ostream& Task::error(const Arena& arena)
{
    return Logger::error(arena) << name << ": ";
}

void Task::addProduct(const Product& product)
{
    products.push_back(product);
}

bool Task::registerTask(const string& name, factory_func create)
{
    tasks()[name] = create;
    return true;
}

const Schema& Task::getSchema(const string& name)
{
    static map<string,Schema> schemas;

    if (schemas.empty())
    {
        Config master(TOPDIR "/desc/schemas");

        vector<pair<string,Config> > configs = master.find<Config>("*");
        for (vector<pair<string,Config> >::iterator i = configs.begin();i != configs.end();++i)
        {
            Config schema = master.get(i->first);
            schemas[i->first] = Schema(schema);
        }
    }

    map<string,Schema>::iterator i = schemas.find(name);
    if (i == schemas.end()) throw logic_error("Cannot find schema for task " + name);
    return i->second;
}

Product& Task::getProduct(const string& name)
{
    for (vector<Product>::iterator i = products.begin();i != products.end();i++)
    {
        if (i->getName() == name) return *i;
    }
    throw logic_error("Product " + name + " not found on task " + this->name);
}

const Product& Task::getProduct(const string& name) const
{
    return const_cast<Task&>(*this).getProduct(name);
}

Task* Task::createTask(const string& type, const string& name, const input::Config& config)
{
    map<string,factory_func>::iterator i = tasks().find(type);

    if (i == tasks().end()) throw logic_error("Task type " + type + " not found");

    return i->second(name, config);
}

void TaskDAG::parseTasks(const string& context, Config& input)
{
    vector<pair<string,string> > sections = input.find<string>("section");

    for (vector<pair<string,string> >::iterator i = sections.begin();i != sections.end();++i)
    {
        {
            Config section = input.get<Config>("section." + i->second);
            parseTasks(context+i->second+".", section);
        }
        input.remove("section");
    }

    vector<pair<string,Config> > taskconfs = input.find<Config>("*");

    for (vector<pair<string,Config> >::iterator i = taskconfs.begin();i != taskconfs.end();++i)
    {
        int num = 0;
        for (vector<pair<Task*,Config> >::iterator t = tasks.begin();t != tasks.end();++t)
        {
            if (t->first->getName().compare(0, context.size(), context) == 0 &&
                t->first->getType() == i->first) num++;
        }

        Config config = i->second.clone();

        string name = context + i->first + (num == 0 ? "" : str(num));
        if (config.exists("name"))
        {
            name = config.get<string>("name");
            if (name.find_first_of(".:") != string::npos)
                Logger::error(Arena()) << "Task names may not contain any of '.:' (" << name << ")" << endl;
            name = context+name;
            config.remove("name");
        }

        for (vector<pair<Task*,Config> >::iterator t = tasks.begin();t != tasks.end();++t)
        {
            if (t->first->getName() == name)
                Logger::error(Arena()) << "More than one task with name " << name << endl;
        }

        while (config.exists("using")) config.remove("using");

        Task::getSchema(i->first).apply(config);
        tasks.push_back(make_pair(Task::createTask(i->first, name, config), i->second.clone()));
    }
}

TaskDAG::TaskDAG(const string& file)
{
    Config input(file);
    parseTasks("", input);
}

TaskDAG::~TaskDAG()
{
    for (vector<pair<Task*,Config> >::iterator i = tasks.begin();i != tasks.end();++i) delete i->first;
    tasks.clear();
}

void TaskDAG::addTask(Task* task, const Config& config)
{
    tasks.push_back(make_pair(task,config));
}

void TaskDAG::satisfyRemainingRequirements(const Arena& world)
{
    /*
     * Attempy to satisfy remaining task requirements greedily.
     * If we are not careful, this could produce cycles.
     */
    for (vector<pair<Task*, Config> >::iterator t1 = tasks.begin();t1!=tasks.end();++t1)
    {
        string context1;
        size_t sep = t1->first->getName().find_last_of(".");
        if (sep!=string::npos)
        {
            context1 = t1->first->getName().substr(0, sep+1);
        }
        for (vector<Product>::iterator p1 = t1->first->getProducts().begin();p1!=t1->first->getProducts().end();++p1)
        {
            for (vector<Requirement>::iterator r1 = p1->getRequirements().begin();r1!=p1->getRequirements().end();++r1)
            {
                if (r1->isFulfilled())
                    continue;

                if (r1->getType()=="double")
                    Logger::error(world)<<"scalar requirements must be explicitly fulfilled"<<endl;

                for (vector<pair<Task*, Config> >::iterator t2 = tasks.begin();t2!=tasks.end();++t2)
                {
                    string context2;
                    size_t sep = t2->first->getName().find_last_of(".");
                    if (sep!=string::npos)
                    {
                        context2 = t2->first->getName().substr(0, sep+1);
                    }
                    if (context1.compare(0, context2.size(), context2)!=0)
                        continue;

                    for (vector<Product>::iterator p2 = t2->first->getProducts().begin();p2!=t2->first->getProducts().end();++p2)
                    {
                        if (r1->isFulfilled())
                            continue;

                        if (r1->getType()==p2->getType())
                        {
                            r1->fulfil(*p2);
                        }
                        for (vector<Requirement>::iterator r2 = p2->getRequirements().begin();r2!=p2->getRequirements().end();++r2)
                        {
                            if (r1->isFulfilled())
                                continue;

                            if (!r2->isFulfilled())
                                continue;

                            if (r1->getType()==r2->getType())
                            {
                                r1->fulfil(r2->get());
                            }
                        }
                    }
                }
                if (!r1->isFulfilled())
                    Logger::error(world)<<"Could not fulfil requirement "<<r1->getName()<<" of task "<<t1->first->getName()<<endl;
            }
        }
    }
}

void TaskDAG::satisfyExplicitRequirements(const Arena& world)
{
    /*
     * Hook up explicitly fulfilled requirements.
     */
    for (vector<pair<Task*, Config> >::iterator t1 = tasks.begin();t1!=tasks.end();++t1)
    {
        string context;

        size_t sep = t1->first->getName().find_last_of(".");
        if (sep!=string::npos)
        {
            context = t1->first->getName().substr(0, sep+1);
        }

        vector<pair<string, string> > usings = t1->second.find<string>("using");

        for (vector<pair<string, string> >::iterator u = usings.begin();u!=usings.end();++u)
        {
            vector<Requirement*> reqs;

            for (vector<Product>::iterator p = t1->first->getProducts().begin();p!=t1->first->getProducts().end();++p)
            {
                for (vector<Requirement>::iterator r = p->getRequirements().begin();r!=p->getRequirements().end();++r)
                {
                    if (r->getName()==u->second)
                    {
                        if (!reqs.empty()&&reqs.back()->getType()!=r->getType())
                            Logger::error(world)<<"Multiple requirements named "<<u->second<<" with different types"<<endl;
                        reqs.push_back(&(*r));
                    }
                }
            }

            if (reqs.empty())
                Logger::error(world)<<"No requirement "+u->second+" found on task "+t1->first->getName()<<endl;

            Product p("double", u->second);
            Product* fulfiller = NULL;

            if (t1->second.exists("using."+u->second+".="))
            {
                if (reqs.back()->getType()!="double")
                    Logger::error(world)<<"Attempting to specify a non-scalar requirement by value"<<endl;
                p.put(new Scalar(world, t1->second.get<double>("using."+u->second+".=")));
                fulfiller = &p;
            }
            else
            {
                string from = t1->second.get<string>("using."+u->second+".from");
                string task, req;

                size_t sep = from.find(':');
                if (sep==string::npos)
                {
                    task = from;
                    req = u->second;
                }
                else
                {
                    task = from.substr(0, sep);
                    req = from.substr(sep+1);
                }

                sep = task.find('.');
                if (sep==string::npos)
                {
                    task = context+task;
                }

                if (task[0]=='.')
                    task = task.substr(1);

                for (vector<pair<Task*, Config> >::iterator t2 = tasks.begin();t2!=tasks.end();++t2)
                {
                    if (t2->first->getName()==task)
                    {
                        for (vector<Product>::iterator p2 = t2->first->getProducts().begin();p2!=t2->first->getProducts().end();++p2)
                        {
                            if (p2->getName()==req)
                            {
                                if (p2->getType()!=reqs.back()->getType())
                                    Logger::error(world)<<"Product "<<task<<"."<<req<<" is wrong type for requirement "<<t1->first->getName()<<"."<<req<<endl;
                                fulfiller = &(*p2);
                            }
                        }

                        if (fulfiller==NULL)
                            Logger::error(world)<<"Product "+req+" not found on task "+task<<endl;
                    }
                }

                if (fulfiller==NULL)
                    Logger::error(world)<<"Task "+task+" not found"<<endl;
            }

            for (vector<Requirement*>::iterator r = reqs.begin();r!=reqs.end();++r)
            {
                (*r)->fulfil(*fulfiller);
            }

            t1->second.remove("using");
        }
    }
}

void TaskDAG::execute(Arena& world)
{
    satisfyExplicitRequirements(world);
    satisfyRemainingRequirements(world);

    //TODO: check for cycles

    /*
     * Successively search for executable tasks
     */
    while (true)
    {
        vector<Task*> to_execute;

        for (vector<pair<Task*,Config> >::iterator t = tasks.begin();;)
        {
            if (t == tasks.end()) break;

            bool can_execute = true;

            for (vector<Product>::iterator p = t->first->getProducts().begin();p != t->first->getProducts().end();++p)
            {
                for (vector<Requirement>::iterator r = p->getRequirements().begin();r != p->getRequirements().end();++r)
                {
                    if (!r->exists())
                    {
                        can_execute = false;
                        break;
                    }
                }
                if (!can_execute) break;
            }

            if (can_execute)
            {
                to_execute.push_back(t->first);
                t = tasks.erase(t);
            }
            else
            {
                ++t;
            }
        }

        if (to_execute.empty()) break;

        for (vector<Task*>::iterator t = to_execute.begin();t != to_execute.end();++t)
        {
            Logger::log(world) << "Starting task: " << (*t)->getName() << endl;
            Timer timer;

            bool success = true;
            string error;

            timer.start();
            try
            {
                (*t)->run(*this, world);
            }
            catch (runtime_error& e)
            {
                success = false;
                error = e.what();
            }
            timer.stop();

            double dt = timer.seconds(world);
            double gflops = timer.gflops(world);
            Logger::log(world) << "Finished task: " << (*t)->getName() <<
                       " in " << std::fixed << std::setprecision(3) << dt << " s" << endl;
            Logger::log(world) << "Task: " << (*t)->getName() <<
                       " achieved " << std::fixed << std::setprecision(3) << gflops << " Gflops/sec" << endl;

            if (!success)
            {
                while (t != to_execute.end()) delete *t++;
                throw runtime_error(error);
            }

            for (vector<Product>::iterator p = (*t)->getProducts().begin();p != (*t)->getProducts().end();++p)
            {
                if (p->isUsed() && !p->exists())
                    Logger::error(world) << "Product " << p->getName() <<
                                            " of task " << (*t)->getName() <<
                                            " was not successfully produced" << endl;
            }

            delete *t;
        }
    }

    if (!tasks.empty())
    {
        Logger::error(world) << "Some tasks were not executed" << endl;
    }
}

CompareScalars::CompareScalars(const string& name, const Config& config)
: Task("compare", name)
{
    tolerance = config.get<double>("tolerance");

    vector<Requirement> reqs;
    reqs.push_back(Requirement("double", "val1"));
    reqs.push_back(Requirement("double", "val2"));
    addProduct(Product("bool", "match", reqs));
}

void CompareScalars::run(TaskDAG& dag, const Arena& arena)
{
    double val1 = get<Scalar>("val1");
    double val2 = get<Scalar>("val2");

    bool match = abs(val1-val2) < tolerance;

    if (match)
    {
        log(arena) << "passed" << endl;
    }
    else
    {
        error(arena) << "failed: " << fixed <<
                setprecision((int)(0.5-log10(tolerance))) << val1 << " vs " << val2 << endl;
    }

    put("match", new Boolean(arena, match));
}

REGISTER_TASK(CompareScalars,"compare");

}
}
