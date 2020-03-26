//  Project             :  DispatcherChallenge
//  Author              :  Patrik Michlo
//  Date of creation    :  2020-03-25
//  Workfile            :  main.cpp
//--------------------------------------------------------------------------
//  Description: Main function for a DispatcherChallenge project
//--------------------------------------------------------------------------
#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

//
// supporting tools and software
//
// Validate and test your json commands
// https://jsonlint.com/

// RapidJSON : lots and lots of examples to help you use it properly
// https://github.com/Tencent/rapidjson
//

// std::function
// std::bind
// std::placeholders
// std::map
// std::make_pair

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

bool g_done = false;

const char* PAYLOAD_KEY = "payload";
const char* REASON_KEY = "reason";
const char* NAME_KEY = "name";
const char* POSITION_KEY = "position";
const char* COMMAND_KEY = "command";
const char* DESCRIPTION_KEY = "description";

static int employeeID = 0;
//
// TEST COMMANDS
//

auto help_command = R"(
{
  "command": "help",
  "payload": [
    {
      "command" : "help",
      "description": "Print this Help."
    },
    {
      "command" : "exit",
      "description": "Exit this program."
    },
    {
      "command" : "add",
      "description": "Add employees from JSON to data container."
    },
    {
      "command" : "remove",
      "description": "Remove employee by specific ID given by JSON."
    },
    {
      "command" : "print",
      "description": "Print list of employees."
    }
  ]
})";

auto exit_command = R"(
 {
  "command":"exit",
  "payload": {
     "reason":"Exiting program on user request."
  }
 }
)";

auto add_command = R"(
{
  "command": "add",
  "payload": [
    {
      "name" : "Peter",
      "position": "C++ Developer"
    },
    {
      "name": "Ivan",
      "position": "Java Developer"
    },
    {
      "name": "Michal",
      "position": "UI Designer"
    }
  ]
})";

auto print_command = R"(
{
  "command": "print",
  "payload": "Printing list of employees:\n"
})";

auto remove_command = R"(
{
  "command": "print",
  "payload": 3
})";

class Employee
{
public:
  typedef std::vector<std::unique_ptr<Employee>> Employees;

  Employee(int id, std::string name, std::string position) : id_(id), name_(name), position_(position) {}
  int getId() { return id_; }
  std::string getName() { return name_; }
  std::string getPosition() { return position_; }

private:
  int id_;
  std::string name_;
  std::string position_;
};

class Controller {
public:
  Controller(Employee::Employees& employees) : employees_(employees) { }

  bool help(rapidjson::Value &payload)
  {
    cout << "Controller::help: command: " << endl;

    if (!payload.IsArray())
    {
      cout << "Controller::help: command not successfull. \n";
      return false;
    }

    for (const auto& object : payload.GetArray())
    {
      assert(object.IsObject());
      cout << object[COMMAND_KEY].GetString()  << " - " << object[DESCRIPTION_KEY].GetString() << endl;
    }

    return true;
  }

  bool exit(rapidjson::Value &payload)
  {
    cout << "Controller::exit: command: \n";

    if (payload.HasMember(REASON_KEY))
    {
      cout << payload[REASON_KEY].GetString() << endl;
      g_done = true;

      return true;
    }

    cout << "Exit command not successful." << endl;
    return false;
  }

  // implement 3-4 more commands
  bool add(rapidjson::Value &payload)
  {
    cout << "Controller::add: command. \n";

    if (!payload.IsArray())
    {
      cout << "Controller::add: command not successfull. \n";
      return false;
    }

    for (const auto& object : payload.GetArray())
    {
      assert(object.IsObject());
      auto employee = std::make_unique<Employee>(Employee(++employeeID, object[NAME_KEY].GetString(), object[POSITION_KEY].GetString()));
      employees_.emplace_back(std::move(employee));
    }

    cout << "Controller::add: command. Employees added. \n";

    return true;
  }

  bool print(rapidjson::Value &payload)
  {
    cout << "Controller::print: command. \n";

    if (!payload.IsString())
    {
      cout << "Controller::print: command not successfull. \n";
      return false;
    }

    if (employees_.empty())
    {
      cout << "Empty list of employees" << endl;
      return false;
    }

    cout << payload.GetString() << endl;

    for (const auto& employee: employees_)
    {
      cout << "Employee: " << employee->getId() << ". " << employee->getName() << " - " << employee->getPosition() << endl;
    }

    return true;
  }

  bool remove(rapidjson::Value &payload)
  {
    cout << "Controller::remove: command. \n";

    if (!payload.IsInt())
    {
      cout << "Controller::remove: command not successfull. \n";
      return false;
    }

    int empID = payload.GetInt();
    const auto& it = std::find_if(employees_.cbegin(), employees_.cend(), [empID](const auto& employee) {
      return employee->getId() == empID;
    });

    if (it == employees_.end())
    {
      cout << "Employee with the given ID does not exist." << endl;
      return false;
    }

    employees_.erase(it);

    cout << "Controller::remove: command. Employee removed.\n";

    return true;
  }

private:
  // Container for storing parsed employees
  Employee::Employees& employees_;
};

// gimme ... this is actually tricky
// Bonus Question: why did I type cast this?
// Patrik Michlo: Well, actually the only reason what comes to my mind is the readability of the code - the fact
// that we can refer to that definition just by CommandHandler name
typedef std::function<bool(rapidjson::Value &)> CommandHandler;

class CommandDispatcher {
public:
  // ctor - need impl
  // Patrik Michlo: The way I did it does not need impl here.
  CommandDispatcher()
  {
  }

  // dtor - need impl
  // Patrik Michlo: The way I did it does not need impl here.
  virtual ~CommandDispatcher()
  {
    // question why is it virtual? Is it needed in this case?
    // Patrik Michlo: There is no need to make this destructor virtual unless we want to derive from this class.
  }

  bool addCommandHandler(std::string command, CommandHandler handler)
  {
    cout << "CommandDispatcher: addCommandHandler: " << command << std::endl;

    auto result = command_handlers_.emplace(command, handler);

    return result.second;
  }

  bool dispatchCommand(std::string command)
  {
    cout << "COMMAND: " << command << endl;

    const auto& it = command_handlers_.find(command);

    if (it == command_handlers_.end())
    {
      cout << "COMMAND: " << command << " not supported. " << endl;
      return false;
    }

    std::string commandJson;
    if (command == "help")
    {
      commandJson = help_command;
    }
    else if (command == "exit")
    {
      commandJson = exit_command;
    }
    else if (command == "add")
    {
      commandJson = add_command;
    }
    else if (command == "print")
    {
      commandJson = print_command;
    }
    else if (command == "remove")
    {
      commandJson = remove_command;
    }

    Document document;
    document.Parse(commandJson.c_str());
    assert(document.HasMember(PAYLOAD_KEY));

    // Pass the value to CommandHandler
    it->second(document[PAYLOAD_KEY]);

    return true;
  }

private:

  // gimme ...
  std::map<std::string, CommandHandler> command_handlers_;

  // another gimme ...
  // Question: why delete these?
  // Patrik Michlo: There is no sense to have multiple instances of CommandDispatcher,so deleting copy constructor and operator "=" is OK.
  // delete unused constructors
  CommandDispatcher (const CommandDispatcher&) = delete;
  CommandDispatcher& operator= (const CommandDispatcher&) = delete;

};

int main()
{
  std::cout << "COMMAND DISPATCHER: STARTED" << std::endl;

  Employee::Employees employees;

  CommandDispatcher command_dispatcher;
  Controller controller(employees);                 // controller class of functions to "dispatch" from Command Dispatcher


  // Implement
  // add command handlers in Controller class to CommandDispatcher using addCommandHandler
  command_dispatcher.addCommandHandler("help", std::bind(&Controller::help, controller, std::placeholders::_1));
  command_dispatcher.addCommandHandler("exit", std::bind(&Controller::exit, controller, std::placeholders::_1));
  command_dispatcher.addCommandHandler("add", std::bind(&Controller::add, controller, std::placeholders::_1));
  command_dispatcher.addCommandHandler("print", std::bind(&Controller::print, controller, std::placeholders::_1));
  command_dispatcher.addCommandHandler("remove", std::bind(&Controller::remove, controller, std::placeholders::_1));

  // gimme ...
  // command line interface
  string command;
  while (!g_done) {
    cout << "Type \"help\" to view list of commands." << endl;
    cout << "\tenter command : ";
    getline(cin, command);
    command_dispatcher.dispatchCommand(command);
  }

  std::cout << "COMMAND DISPATCHER: ENDED" << std::endl;
  return 0;
}