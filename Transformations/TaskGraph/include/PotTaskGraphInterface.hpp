#include <sstream>
#include <vector>
enum AccessType{
    AccessRead,
    AccessWrite
};

class PotTask{
public:
    PotTask(const int inTaskId) : inTaskId(inTaskId) {};
    void addParam(AccessType inAccess, const std::string& inDepId){
        params.push_back(std::make_pair(inAccess, inDepId));}
    std::string dump()
    {
        std::stringstream ssPrint;
        int count =0;
        for (auto& p : params){
            ssPrint<<"Param "<<count<< " ";
            switch (p.first){
                case AccessType::AccessRead:
                    ssPrint << "Read";
                    break;
                case AccessType::AccessWrite:
                    ssPrint << "Write";
                    break;
            }
            ssPrint<<" " << p.second << " "<< "\n";
        count++;
    }
    return ssPrint.str();
    }
private:
    std::vector<std::pair<AccessType, std::string>> params;
    const int inTaskId;
};

class PotTaskGraph{
public:
    PotTaskGraph() = default;
    void addTask(PotTask& inTask){
        tasks.push_back(inTask);}
    std::string dump(){
        std::stringstream ssPrint;
        ssPrint << "TaskGraph\n";
        int count=0;
        for (auto& t : tasks){
            ssPrint << "Task "<<count <<"\n"<< t.dump() << "\n";
            count++;
        }return ssPrint.str();}
private:
    std::vector<PotTask> tasks;
};

