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

	std::string State::serialize() const
	{
		if (pointer != nullptr)
		{
			return serializer(pointer);
		}
		return "";
	}

	void State::deserialize(const std::string& value)
	{
		if (deserializer != nullptr)
		{
			deserializer(pointer, data.data(), value);
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
	Node& Node::PortWrapper::getNode() const
	{
		return *node;
	}

	bool Node::PortWrapper::isVoid() const noexcept
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

	Node::PortWrapper& Node::PortWrapper::operator >>(PortWrapper& port)
	{
		connect(port);
		return port;
	}

	Node::PortWrapper& Node::PortWrapper::operator<<(PortWrapper& port)
	{
		port.connect(*this);
		return *this;
	}

	void Node::PortWrapper::connect(const PortWrapper& t)
	{
		if (!isVoid())
			node->connect(*this, t);
	}

	Node::PortWrapper::PortWrapper(Node* n, unsigned p, bool i, bool f)
	{
		node = n;
		port = p;
		input = i;
		flow = f;
	}

	Node::PortWrapper Node::PortWrapper::voidPort()
	{
		return PortWrapper(nullptr, 0, false, false);
	}

	unsigned Node::ConstPortWrapper::getPort() const noexcept
	{
		return port;
	}

	bool Node::ConstPortWrapper::isFlowPort() const noexcept
	{
		return flow;
	}

	bool Node::ConstPortWrapper::isInput() const noexcept
	{
		return input;
	}

	const Node& Node::ConstPortWrapper::getNode() const noexcept
	{
		return *node;
	}

	bool Node::ConstPortWrapper::isVoid() const noexcept
	{
		return node == nullptr;
	}

	bool Node::ConstPortWrapper::operator==(const ConstPortWrapper& t) const noexcept
	{
		return node == t.node && flow == t.flow && input == t.input;
	}

	bool Node::ConstPortWrapper::operator!=(const ConstPortWrapper& t) const noexcept
	{
		return node != t.node || flow != t.flow || input != t.input;
	}

	Node::ConstPortWrapper::ConstPortWrapper(const PortWrapper& p) :
		node(p.isVoid() ? nullptr : &p.getNode()),
		port(p.getPort()),
		flow(p.isFlowPort()),
		input(p.isInput()) { }

	Node::ConstPortWrapper::ConstPortWrapper(const Node* n, unsigned p, bool i, bool f) :
		node(n),
		port(p),
		flow(f),
		input(i) { }

	Node::ConstPortWrapper Node::ConstPortWrapper::voidPort()
	{
		return ConstPortWrapper(nullptr, 0, false, false);
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

	Node::ConstPortWrapper Node::ConstPortTypeWrapper::get(unsigned port) const noexcept
	{
		bool input = false, flow = false;
		switch (type)
		{
		case PortTypeWrapper::Type::FlowOutput:
		{
			input = false;
			flow = true;
			break;
		}
		case PortTypeWrapper::Type::FlowInput:
		{
			input = true;
			flow = true;
			break;
		}
		case PortTypeWrapper::Type::ValueOutput:
		{
			input = false;
			flow = false;
			break;
		}
		case PortTypeWrapper::Type::ValueInput:
		{
			input = true;
			flow = false;
			break;
		}
		}
		return ConstPortWrapper(node, port, input, flow);
	}

	Node::ConstPortTypeWrapper::ConstPortTypeWrapper(const Node* n, PortTypeWrapper::Type t) : node(n), type(t) { }

	const NodeTemplate::Ptr Node::getBase() const noexcept
	{
		return base;
	}

	State* Node::getConfig() const
	{
		return config.get();
	}

	void Node::deserializeInputValue(unsigned id, const std::string& value)
	{
		if (id < inputValues.size())
		{
			inputValues[id] = base->inputs[id].createState();
			inputValues[id]->deserialize(value);
		}
	}

	const PortState* Node::getInputState(unsigned id) const
	{
		if (id < inputValues.size())
		{
			return inputValues.at(id).get();
		}
		return nullptr;
	}

	bool Node::connect(const PortWrapper& source, const PortWrapper& target) noexcept
	{
		//sanity check
		if (source.isVoid() || &source.getNode() != this)
			return false;

		//in-in/out-out connection
		if (!(source.isInput() ^ target.isInput()))
			return false;

		//different port types
		if (source.isFlowPort() ^ target.isFlowPort())
			return false;

		unsigned char v = (source.isInput() ? 1 : 0) | (source.isFlowPort() ? 2 : 0);

		Node& n = target.getNode();

		switch (v)
		{
		case 0: //output
		{
			auto it = outputPorts.find(source.getPort());
			if (it == outputPorts.end())
			{
				it = outputPorts.emplace(std::piecewise_construct, std::forward_as_tuple(source.getPort()), std::tuple<>()).first;
			}

			auto it2 = std::find_if(it->second.begin(), it->second.end(), [&target](const PortWrapper& t)
				{
					return target == t;
				});

			//node not already connected, target has free port
			if (it2 == it->second.end() && target.getNode().inputPorts.find(target.getPort()) == target.getNode().inputPorts.end())
			{
				it->second.emplace_back(target);
				n.inputPorts.emplace(target.getPort(), source);
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
				auto it2 = n.outputPorts.find(target.getPort());
				if (it2 == n.outputPorts.end())
				{
					it2 = n.outputPorts.emplace(std::piecewise_construct, std::forward_as_tuple(target.getPort()), std::tuple<>()).first;
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
				auto it2 = n.flowInputPorts.find(target.getPort());
				if (it2 == n.flowInputPorts.end())
				{
					it2 = n.flowInputPorts.emplace(std::piecewise_construct, std::forward_as_tuple(target.getPort()), std::tuple<>()).first;
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
				it = flowInputPorts.emplace(std::piecewise_construct, std::forward_as_tuple(source.getPort()), std::tuple<>()).first;
			}

			auto it2 = std::find_if(it->second.begin(), it->second.end(), [&target](const PortWrapper& t)
				{
					return target == t;
				});

			if (it2 == it->second.end() && target.getNode().flowOutputPorts.find(target.getPort()) == target.getNode().flowOutputPorts.end()) //node not already connected, target has free port
			{
				it->second.emplace_back(target);
				n.flowOutputPorts.emplace(target.getPort(), source);
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

		//sanity check
		if (&source.getNode() != this)
			return false;

		//in-in/out-out connection
		if (!(source.isInput() ^ target.isInput()) && !voidAll)
			return false;

		//different port types
		if (source.isFlowPort() ^ target.isFlowPort() && !voidAll)
			return false;

		unsigned char v = (source.isInput() ? 1 : 0) | (source.isFlowPort() ? 2 : 0);

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
						i.getNode().inputPorts.erase(i.getPort());
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
							target.getNode().inputPorts.erase(target.getPort());
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

				Node& n = it->second.getNode();

				auto it2 = n.outputPorts.find(it->second.getPort());
				if (it2 != n.outputPorts.end())
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

				Node& n = it->second.getNode();

				auto it2 = n.flowInputPorts.find(it->second.getPort());
				if (it2 != n.flowInputPorts.end())
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
						i.getNode().flowOutputPorts.erase(i.getPort());
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
							target.getNode().flowOutputPorts.erase(target.getPort());
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

	Node::PortWrapper Node::getPort(unsigned id, bool input, bool flow)
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
		return PortWrapper(this, id, input, flow);
	}

	Node::ConstPortWrapper Node::getPort(unsigned id, bool input, bool flow) const
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
		return ConstPortWrapper(this, id, input, flow);
	}

	Node::ConstPortTypeWrapper Node::operator[] (PortType portType) const noexcept
	{
		return ConstPortTypeWrapper(this, portType);
	}

	Node::PortTypeWrapper Node::operator[] (PortType portType) noexcept
	{
		return PortTypeWrapper(this, portType);
	}

	Node::Node(const NodeTemplate::Ptr& nodeTemplate)
	{
		usedInputs = 0;
		base = nodeTemplate;
		config = base->config.createState();
		inputValues.resize(base->inputs.size());
	}

	void Enviroment::clearNodes()
	{
		nodes.clear();
	}

	void Enviroment::clear()
	{
		nodes.clear();
		nodeTemplates.clear();
	}
	std::unique_ptr<XML::Tag> Enviroment::exportToXML() const
	{
		std::unique_ptr<XML::Tag> root = std::make_unique<XML::Tag>();
		root->name = "Enviroment";

		std::unordered_map<Node*, size_t> nodeMappings;
		size_t i = 0;
		for (auto& node : nodes)
		{
			nodeMappings.emplace(node.get(), i);
			++i;
		}

		for (auto& node : nodes)
		{
			NodeTemplate::Ptr base = node->getBase();

			std::unique_ptr<XML::Tag> nodeTag = std::make_unique<XML::Tag>();
			nodeTag->name = "Node";
			nodeTag->attributes.emplace("id", std::to_string(nodeMappings.at(node.get())));
			nodeTag->attributes.emplace("base", base->action);
			//connections
			{
				std::unique_ptr<XML::Tag> portsTag = std::make_unique<XML::Tag>();
				portsTag->name = "Connections";
				for (unsigned p = 0; p < base->flowOutputPoints; ++p)
				{
					Node::PortWrapper port = node->getFlowOutputPortTarget(p);
					if (!port.isVoid())
					{
						std::unique_ptr<XML::Tag> value = std::make_unique<XML::Tag>();
						value->name = "FlowOutputPort";

						value->setAttribute("port", std::to_string(p));
						value->setAttribute("targetId", std::to_string(port.getPort()));
						value->setAttribute("connectedTo", std::to_string(nodeMappings.at(&port.getNode())));

						portsTag->addChild(std::move(value));
					}
				}

				for (unsigned p = 0; p < base->inputs.size(); ++p)
				{
					std::unique_ptr<XML::Tag> value = std::make_unique<XML::Tag>();
					value->name = "InputPort";
					value->setAttribute("port", std::to_string(p));

					Node::PortWrapper port = node->getInputPortTarget(p);
					if (!port.isVoid())
					{
						//port connected to another node
						value->setAttribute("targetId", std::to_string(nodeMappings.at(&port.getNode())));
						value->setAttribute("connectedTo", std::to_string(port.getPort()));

						portsTag->addChild(std::move(value));
					}
					else
					{
						//internal port value
						const PortState* state = node->getInputState(p);
						if (state != nullptr)
						{
							std::string v = state->serialize();

							value->setAttribute("isValue");
							value->addChild(std::make_unique<XML::TextNode>(v));

							portsTag->addChild(std::move(value));
						}
					}
				}
				if (!portsTag->children.empty())
					nodeTag->addChild(std::move(portsTag));
			}
			//config
			{
				State* state = node->getConfig();

				if (state != nullptr && state->isReady())
				{
					std::unique_ptr<XML::Tag> config = std::make_unique<XML::Tag>();
					config->name = "Config";

					std::string v = state->serialize();

					config->addChild(std::make_unique<XML::TextNode>(v));

					nodeTag->addChild(std::move(config));
				}
			}
			root->addChild(std::move(nodeTag));
		}

		return root;
	}
	void Enviroment::importFromXML(const std::unique_ptr<XML::Tag>& tag)
	{
		if (tag->name == "Enviroment")
		{
			std::unordered_map<std::string, NodeTemplate::Ptr> templateMapping;
			for (auto& base : nodeTemplates)
			{
				templateMapping.emplace(base->action, base);
			}
			std::unordered_map<std::string, Node*> mapping;
			//first pass, create nodes and configs
			for (const auto& node : tag->children)
			{
				if (!node->isTextNode())
				{
					XML::Tag* nodeTag = static_cast<XML::Tag*>(node.get());
					XML::Value	idV = nodeTag->getAttribute("id"),
						baseV = nodeTag->getAttribute("base");
					if (!idV.exists() || !baseV.exists())
						continue;

					auto it = templateMapping.find(baseV.val);
					if (it != templateMapping.end())
					{
						Node::Ptr n = createNode(it->second);
						mapping.emplace(idV.val, n.get());

						for (auto& i : nodeTag->children)
						{
							XML::Tag* configTag = dynamic_cast<XML::Tag*>(i.get());
							if (configTag == nullptr)
								continue;
							if (configTag->name == "Config")
							{
								n->getConfig()->deserialize(configTag->formatContents(true));
							}
						}
					}
				}
			}
			//second pass, add connections
			for (const auto& node : tag->children)
			{
				if (!node->isTextNode())
				{
					XML::Tag* nodeTag = static_cast<XML::Tag*>(node.get());
					Node* n = mapping.at(nodeTag->getAttribute("id").val);
					for (auto& i : nodeTag->children)
					{
						if (!i->isTextNode())
						{
							XML::Tag* subTag = static_cast<XML::Tag*>(i.get());
							if (subTag->name == "Connections")
							{
								for (auto& j : subTag->children)
								{
									if (!j->isTextNode())
									{
										XML::Tag* portTag = static_cast<XML::Tag*>(j.get());
										if (portTag->name == "FlowOutputPort")
										{
											XML::Value port = portTag->getAttribute("port"), targetId = portTag->getAttribute("targetId"), portT = portTag->getAttribute("connectedTo");
											if (port.exists() && targetId.exists() && portT.exists())
											{
												size_t p = std::stoull(port.val);
												size_t pT = std::stoull(portT.val);

												auto it = mapping.find(targetId.val);
												if (it != mapping.end())
												{
													n->getFlowOutputPort(static_cast<unsigned>(p)).connect(it->second->getFlowInputPort(static_cast<unsigned>(pT)));
												}
											}
										}
										else if (portTag->name == "InputPort")
										{
											if (portTag->getAttribute("isValue").exists())
											{
												XML::Value port = portTag->getAttribute("port");
												if (port.exists())
												{
													size_t p = std::stoull(port.val);
													n->deserializeInputValue(static_cast<unsigned>(p), portTag->formatContents(true));
												}
											}
											else
											{
												XML::Value port = portTag->getAttribute("port"), targetId = portTag->getAttribute("targetId"), portT = portTag->getAttribute("connectedTo");
												if (port.exists() && targetId.exists() && portT.exists())
												{
													size_t p = std::stoull(port.val);
													size_t pT = std::stoull(portT.val);

													auto it = mapping.find(targetId.val);
													if (it != mapping.end())
													{
														n->getInputPort(static_cast<unsigned>(p)).connect(it->second->getOutputPort(static_cast<unsigned>(pT)));
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	Node::Ptr Enviroment::createNode(const NodeTemplate::Ptr& base)
	{
		nodes.emplace_back(std::make_shared<Node>(base));
		return nodes.back();
	}
	NodeTemplate::Ptr Enviroment::createTemplate()
	{
		nodeTemplates.emplace_back(std::make_unique<NodeTemplate>());
		return nodeTemplates.back();
	}
	NodeTemplate::Ptr Enviroment::createTemplate(const std::string& name, const std::vector<Port>& inP, const std::vector<Port>& outP, unsigned flowInP, unsigned flowOutP)
	{
		NodeTemplate::Ptr t = createTemplate();

		for (auto& i : inP)
			t->inputs.emplace_back(i);
		for (auto& i : outP)
			t->outputs.emplace_back(i);

		t->flowInputPoints = flowInP;
		t->flowOutputPoints = flowOutP;

		t->action = name;

		return t;
	}
}