enum AccessType{
    AccessRead,
    AccessWrite
};

class PotTask{
public:
    PotTask(const int inTaskId);
    PotTask& addParam(AccessType inAccess, const std::string& inDepId);
};

class PotTaskGraph{
public:
    PotTaskGraph() = default;
    void addTask(PotTask& inTask);
};

