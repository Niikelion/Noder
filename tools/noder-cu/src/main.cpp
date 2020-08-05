#include <iostream>
#include <cstdlib>
#include <cmath>

#include <Noder/NodeCore.hpp>

using namespace std;

template<typename T> using P = Noder::Pointer<T>;
using Port = Noder::Node::PortType;
using StateAccess = Noder::NodeInterpreter::NodeState::Access;

string inputF()
{
    string c;
    cin >> c;
    return c;
}

std::tuple<string,size_t> valueF(StateAccess access)
{
    string value = access.getConfigValue<string>();
    return { value , value.size() };
}

void printF(string in)
{
    cout << in << endl;
}

void printSizeT(size_t n)
{
    cout << n << endl;
}

int main(int argc,char* argv[])
{
    Noder::NodeInterpreter interpreter;
    
    /*
    P<Noder::NodeTemplate> t1 = &interpreter.createTemplateWithStates(valueF);
    P<Noder::NodeTemplate> t2 = &interpreter.createTemplate(printF);
    */

    Noder::NodeTemplate& t1 = interpreter.createTemplateWithStates([](StateAccess access)
        {
            string value = access.getConfigValue<string>();
            return std::make_tuple( value , value.size() );
        });
    Noder::NodeTemplate& t2 = interpreter.createTemplate([](string in)
        {
            cout << in << endl;
        });

    Noder::NodeTemplate& t3 = interpreter.createTemplate(printSizeT);

    t1.config.setType<string>();

    Noder::Node& n1 = interpreter.createNode(t1);
    
    Noder::Node& n2 = interpreter.createNode(t2);

    Noder::Node& n3 = interpreter.createNode(t3);

    n1.getOutputPort(0) >> n2.getInputPort(0);
    n1.getOutputPort(1) >> n3.getInputPort(0);

    n1.getConfig()->setValue<string>("test");
    
    interpreter.calcNode(n2);
    interpreter.calcNode(n3);
    interpreter.resetStates();
    
    n1.getConfig()->setValue<string>("test 2");
    
    interpreter.calcNode(n2);
    interpreter.calcNode(n3);
}