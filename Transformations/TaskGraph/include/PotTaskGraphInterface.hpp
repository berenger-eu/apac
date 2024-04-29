enum AccessType{
    AccessRead,
    AccessWrite
};

class PotTask{
public:
    PotTask(const int inTaskId) : inTaskId(inTaskId) {};
    void addParam(AccessType inAccess, const std::string& inDepId){;}
private:
    const int inTaskId;
};

class PotTaskGraph{
public:
    PotTaskGraph() = default;
    void addTask(PotTask& inTask){;}
};

