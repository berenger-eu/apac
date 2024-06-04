class Point{
    public:
    Point()=delete;
    Point(int x,int y):x(x),y(y){}
    private :
    int x;
    int y;
};



int main()
{
    int a(5);
    int b=5,c=5,d(4);
    Point p(4,5),p2(4,3);
    // Point p2;
    return 0;
}