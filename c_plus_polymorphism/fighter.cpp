#include <iostream>
using namespace std;


class Fighter
{
    public:
        virtual int power() {
            return 5;
        }
		
    protected:
    private:
};

class FirstFighter:public Fighter
{
    public:
        virtual int power() {
            return 10;
        }
		
    protected:
    private:
};

class SecondFighter:public FirstFighter
{
    public:
       virtual int power() {
          return 20;
       }
	   
    protected:
    private:
};

class EnemyFighter
{
    public:
       int attack() {
           return 15;
       }
	   
    protected:
    private:
};

void playobj(Fighter *f,EnemyFighter *ef)
{
    if (f->power() > ef->attack()) { //f->power()函数调用将会有多态发生，同一个函数，根据传入不同对象，产生不同动作
        cout << "我军胜利" << endl;
    } else {
        cout << "我军挂掉..." << endl;
    }
}


int main(void)
{
    FirstFighter ff;
    SecondFighter sf;
    EnemyFighter ef;

    //1. 方法一：根本不去使用多态
    /*if (ff.power() > ef.attack()) {
        cout << "我军胜利" << endl;
    } else {
        cout << "我军挂掉..." << endl;
    }

    if (sf.power() > ef.attack()) {
        cout << "我军胜利" << endl;
    } else {
        cout << "我军挂掉..." << endl;
    }*/

    //2. 方法二：使用多态方式，通过传入不同对象指针，执行不同类的函数
    playobj(&ff, &ef);
    playobj(&sf, &ef); //这种写法比第一种写法高级很多

    //cout << "sccess..." << endl;
    return 0;
}
