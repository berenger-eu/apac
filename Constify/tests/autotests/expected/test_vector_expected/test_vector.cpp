#include <vector>

void fun(const std::vector<int>  vect)
{
    ;
}
int main()
{
    std::vector<int>vect1({5,4,6,3});
    vect1.push_back(5);
    vect1.at(1);
    vect1.push_back(50);
    const std::vector<int>  vect2;
    vect2.size();
    
    const std::vector<int>  vect4({8,8,8});
    
    const std::vector<int> *const  vectPtr=new std::vector<int>;
    std::vector<int>* vectPtr2=new std::vector<int>;
    vectPtr2->push_back(5);
    const std::vector<int>*const vectPtr3=new std::vector<int>({8,8,8});
    
    using namespace std;
    const vector<int>   vect3;
    
    
    
    return 0;
}