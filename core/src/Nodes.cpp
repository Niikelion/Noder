#include "noder/Nodes.hpp"
#include <stdexcept>
#include <algorithm>

namespace Noder
{
	const char* TypeMismatchException::what() const noexcept
	{
		return msg.c_str();
	}

	TypeMismatchException::TypeMismatchException(const std::type_index& source, const std::type_index& target) noexcept
	{
		msg = std::string("Types does not match(source:") + source.name() + ",target:" + target.name() + ")";
	}

	bool State::isReady() const noexcept
	{
		return pointer != nullptr;
	}

	void State::reset()
	{
		if (pointer != nullptr)
		{
			deleter(pointer);
			pointer = nullptr;
		}
	}

	State::~State()
	{
		if (pointer != nullptr)
			deleter(pointer);
	}

	std::type_index Port::getType() const noexcept
	{
		return type;
	}

	std::unique_ptr<State> Port::createState() const
	{
		if (factory != nullptr)
		{
			return factory();
		}
		return std::unique_ptr<State>();
	}

	bool NodeTemplate::isFlowNode() const noexcept
	{
		return flowInputPoints > 0 || flowOutputPoints > 0;
	}
	bool NodeTemplate::isPureFlowNode() const noexcept
	{
		return isFlowNode() && !isComputationalNode();
	}
	bool NodeTemplate::isComputationalNode() const noexcept
	{
		return inputs.size() > 0 || outputs.size() > 0;
	}
	bool NodeTemplate::isPureComputationalNode() const noexcept
	{
		return isComputationalNode() && !isFlowNode();
	}

	unsigned Node::PortWrapper::getPort() const noexcept
	{
		return port;
	}
	bool Node::PortWrapper::isFlowPort() const noexcept
	{
		return flow;
	}
	bool Node::PortWrapper::isInput() const noexcept
	{
		return input;
	}
	const Node* Node::PortWrapper::getNode() const noexcept
	{
		return node;
	}

	Node* Node::PortWrapper::getNode() noexcept
	{
		return const_cast<Node*>(node);
	}

	bool Node::PortWrapper::isVoid() noexcept
	{
		return node == nullptr;
	}

	bool Node::PortWrapper::operator == (const PortWrapper& t) const noexcept
	{
		return node == t.node && port == t.port && flow == t.flow && input == t.input;
	}

	bool Node::PortWrapper::operator != (const PortWrapper& t) const noexcept
	{
		return node != t.node || port != t.port || flow != t.flow || input != t.input;
	}

	Node::PortWrapper Node::PortWrapper::operator >>(const PortWrapper& port)
	{
		connect(port);
		return port;
	}

	void Node::PortWrapper::connect(const PortWrapper& t)
	{
		if (!isVoid())
			node->connect(*this, t);
	}

	Node::PortWrapper::PortWrapper(const Node* n, unsigned p, bool i, bool f)
	{
		node = const_cast<Node*>(n);
		port = p;
		input = i;
		flow = f;
	}

	Node::PortWrapper Node::PortWrapper::voidPort()
	{
		return PortWrapper(nullptr, 0, false, false);
	}

	Node::PortWrapper Node::PortTypeWrapper::get(unsigned port) const noexcept
	{
		bool input = false, flow = false;
		switch (type)
		{
		case Type::FlowOutput:
		{
			input = false;
			flow = true;
			break;
		}
		case Type::FlowInput:
		{
			input = true;
			flow = true;
			break;
		}
		case Type::ValueOutput:
		{
			input = false;
			flow = false;
			break;
		}
		case Type::ValueInput:
		{
			input = true;
			flow = false;
			break;
		}
		}
		return PortWrapper(node, port, input, flow);
	}

	Node::PortTypeWrapper::PortTypeWrapper(Node* n, Type t)
	{
		node = n;
		type = t;
	}

	const NodeTemplate* Node::getBase() const noexcept
	{
		return base;
	}

	State* Node::getConfig()
	{
		return config.get();
	}

	bool Node::connect(const PortWrapper& source, const PortWrapper& target) noexcept
	{
		if (source.getNode() != this || target.getNode() == nullptr)
			return false;

		if (!(source.isInput() ^ target.isInput())) //in-in/out-out connection
			return false;
		
		if (source.isFlowPort() ^ target.isFlowPort()) //different port types
			return false;

		unsigned char v = (source.isInput() ? 1 : 0) | (source.isFlowPort() ? 2 : 0);

		Node* n = const_cast<Node*>(target.getNode());

		switch (v)
		{
		case 0: //output
		{
			auto it = outputPorts.find(source.getPort());
			if (it == outputPorts.end())
			{
				it = outputPorts.emplace(std::piecewise_construct,std::forward_as_tuple(source.getPort()),std::tuple<>()).first;
			}

			auto it2 = std::find_if(it->second.begin(), it->second.end(), [&target](const PortWrapper& t) 
				{
					return target == t;
				});

			if (it2 == it->second.end() && target.getNode()->inputPorts.find(target.getPort()) == target.getNode()->inputPorts.end()) //node not already connected, target has free port
			{
				it->second.emplace_back(target);
				n->inputPorts.emplace(target.getPort(), source);
			}
			else
			{
				return false;
			}

			break;
		}
		case 1: //input
		{
			auto it = inputPorts.find(source.getPort());
			if (it == inputPorts.end()) //can only connect if not already
			{
				inputPorts.emplace(source.getPort(), target);
				auto it2 = n->outputPorts.find(target.getPort());
				if (it2 == n->outputPorts.end())
				{
					it2 = n->outputPorts.emplace(std::piecewise_construct,std::forward_as_tuple(target.getPort()),std::tuple<>()).first;
				}

				it2->second.emplace_back(source);
			}
			else
			{
				return false;
			}
			break;
		}
		case 2: //flow output
		{
			auto it = flowOutputPorts.find(source.getPort());
			if (it == flowOutputPorts.end()) //can only connect if not already
			{
				flowOutputPorts.emplace(source.getPort(), target);
				auto it2 = n->flowInputPorts.find(target.getPort());
				if (it2 == n->flowInputPorts.end())
				{
					it2 = n->flowInputPorts.emplace(std::piecewise_construct,std::forward_as_tuple(target.getPort()),std::tuple<>()).first;
				}

				it2->second.emplace_back(source);
			}
			else
			{
				return false;
			}
			break;
		}
		case 3: //flow input
		{
			auto it = flowInputPorts.find(source.getPort());
			if (it == flowInputPorts.end())
			{
				it = flowInputPorts.emplace(std::piecewise_construct,std::forward_as_tuple(source.getPort()),std::tuple<>()).first;
			}

			auto it2 = std::find_if(it->second.begin(), it->second.end(), [&target](const PortWrapper& t)
				{
					return target == t;
				});

			if (it2 == it->second.end() && target.getNode()->flowOutputPorts.find(target.getPort()) == target.getNode()->flowOutputPorts.end()) //node not already connected, target has free port
			{
				it->second.emplace_back(target);
				n->flowOutputPorts.emplace(target.getPort(), source);
			}
			else
			{
				return false;
			}
			break;
		}
		default:	//no other options possible
			return false;
		}
		if (v == 3)
		{
			usedInputs++;
		}
		return true;
	}

	bool Node::disconnect(const PortWrapper& source, const PortWrapper& target) noexcept
	{
		bool voidAll = target == PortWrapper::voidPort();

		if (source.getNode() != this)
			return false;

		if (!(source.isInput() ^ target.isInput()) && !voidAll) //in-in/out-out connection
			return false;

		if (source.isFlowPort() ^ target.isFlowPort() && !voidAll) //different port types
			return false;

		unsigned char v = (source.isInput() ? 1 : 0) | (source.isFlowPort() ? 2 : 0);

		Node* n = const_cast<Node*>(target.getNode());

		switch (v)
		{
		case 0: //output
		{
			auto it = outputPorts.find(source.getPort());
			if (it == outputPorts.end())
			{
				return false;
			}
			else
			{
				if (it->second.size() == 0)
					return false;
				if (voidAll)
				{
					for (const auto& i : it->second)
					{
						Node* t = const_cast<Node*>(i.getNode());
						t->inputPorts.erase(i.getPort());
					}
					it->second.clear();
				}
				else
				{
					auto it2 = std::find_if(it->second.begin(), it->second.end(), [&target](const PortWrapper& t) 
						{
							return target == t;
						});
					if (it2 == it->second.end())
					{
						return false;
					}
					else
					{
						if (*it2 == target)
						{
							n->inputPorts.erase(target.getPort());
							it->second.erase(it2);
						}
						else
						{
							return false;
						}
					}
				}
			}
			break;
		}
		case 1: //input
		{
			auto it = inputPorts.find(source.getPort());
			if (it != inputPorts.end())
			{
				if (!voidAll && it->second != target)
				{
					return false;
				}

				n = const_cast<Node*>(it->second.getNode());

				auto it2 = n->outputPorts.find(it->second.getPort());
				if (it2 != n->outputPorts.end())
				{
					auto it3 = std::find(it2->second.begin(), it2->second.end(), source);
					if (it3 != it2->second.end())
					{
						it2->second.erase(it3);
						inputPorts.erase(it);
					}
					else
					{
						return false;
					}
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}
			break;
		}
		case 2: //flow output
		{
			auto it = flowOutputPorts.find(source.getPort());
			if (it != flowOutputPorts.end())
			{
				if (!voidAll && it->second != target)
				{
					return false;
				}

				n = const_cast<Node*>(it->second.getNode());

				auto it2 = n->flowInputPorts.find(it->second.getPort());
				if (it2 != n->flowInputPorts.end())
				{
					auto it3 = std::find(it2->second.begin(), it2->second.end(), source);
					if (it3 != it2->second.end())
					{
						it2->second.erase(it3);
						flowOutputPorts.erase(it);
					}
					else
					{
						return false;
					}
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}
			break;
		}
		case 3: //flow input
		{
			auto it = flowInputPorts.find(source.getPort());
			if (it == flowInputPorts.end())
			{
				return false;
			}
			else
			{
				if (it->second.size() == 0)
					return false;
				if (voidAll)
				{
					for (const auto& i : it->second)
					{
						Node* t = const_cast<Node*>(i.getNode());
						t->flowOutputPorts.erase(i.getPort());
					}
					it->second.clear();
				}
				else
				{
					auto it2 = std::find_if(it->second.begin(), it->second.end(), [&target](const PortWrapper& t)
						{
							return target == t;
						});
					if (it2 == it->second.end())
					{
						return false;
					}
					else
					{
						if (*it2 == target)
						{
							n->flowOutputPorts.erase(target.getPort());
							it->second.erase(it2);
						}
						else
						{
							return false;
						}
					}
				}
			}
			break;
		}
		default:	//no other options possible
			return false;
		}
		if (v == 3)
		{
			usedInputs--;
		}
		return true;
	}

	Node::PortWrapper Node::getPort(unsigned id, bool input, bool flow) const
	{
		if (input)
		{
			if (flow)
			{
				if (id >= getBase()->flowInputPoints)
					throw std::out_of_range("Invalid port number");
			}
			else
			{
				if (id >= getBase()->inputs.size())
					throw std::out_of_range("Invalid port number");
			}
		}
		else
		{
			if (flow)
			{
				if (id >= getBase()->flowOutputPoints)
					throw std::out_of_range("Invalid port number");
			}
			else
			{
				if (id >= getBase()->outputs.size())
					throw std::out_of_range("Invalid port number");
			}
		}
		return PortWrapper(this,id,input,flow);
	}

	Node::PortTypeWrapper Node::operator[](PortType portType) const noexcept
	{
		return PortTypeWrapper(const_cast<Node*>(this),portType);
	}

	Node::Node(NodeTemplate* nodeTemplate)
	{
		usedInputs = 0;
		base = nodeTemplate;
		config = std::make_unique<State>(base->config.getType(),base->config.getSize());
	}

	void Enviroment::clear()
	{
		nodes.clear();
		node_templates.clear();
	}
}