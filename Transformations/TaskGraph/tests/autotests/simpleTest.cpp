//   #include <memory>
//  #include <vector> 
class Point{
public:
    Point() =default;
    void setX(int t)
    {
        x = 4;
    }
    private:
    int x;
    int y;
};
/*
int gun(int& b)
{
    int i;
    i=4;
    i++;
    if(2)
    {
        i++;
        i=4;
    }
    else if(3)
    {
        i++;
        i=4;
    }
    else if(2)
    {
         b=5;
        int b;
        b=0;
        b++;
    }
    b=5;
    b=i;
    b+=4;
    int j;
    j=4*5+4;
    return 1;
}

int fun(int& b)
{
    int i;
    i=4;
    i++;
    gun(b);
    b=i;
    return gun(b);
}*/
 /*
void ptrtest(std::shared_ptr<int> ptr,int &k,int* p)
{
    *ptr = 4;
    k++;
    *p=4;
}*/
int main()
{
    int i;
    i=4;
    
    int j;
  //  std::shared_ptr<int> ptr;
   // ptr = std::make_shared<int>(i);
   // ptr.get();
   // *ptr = 4;
    j=i;
    i++;
    Point po;
    po.setX(4);
    po.setX(8);
    po.setX(i);
    po.setX(j);
    // ptrtest(ptr,j,&i);
    /*
    for(int k=0;k<4;k++)
    {
        i++;
        j++;
        for(int l=0;l<4;l++)
        {
            i++;
            j++;
        }
        for(int m=0;m<4;m++)
        {
            ;
        }
        int a;
        a++;
    }
    
    i=41;
    j++;
    i=4;
    for (int k=0;k<4;k++)
    {
        i++;
        j++;
    }
    j=4;
    i=4+i+j;
  */  
    return 0;
}