#pragma once
//CuiSyncia:20130125
#include <iostream>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/weak_ptr.hpp>
#include "Neuria/Neuria.h"
#include "command/EchoCommand.h"
#include "command/FileCommand.h"
#include "command/FileKeyHashCommand.h"
#include "command/HopCount.h"
#include "database/FileKeyHashDb.h"
#include "database/FileSystemPath.h"
namespace syncia
{
auto CreateNodeIdFromHostNameAndPortNumber(
		const neuria::network::HostName& host_name,
		const neuria::network::PortNumber& port_num) -> database::NodeId {
	return database::NodeId(host_name.ToString()+":"+port_num.ToString());
}

auto CreateHostNameAndPortNumberFromNodeId(
		const database::NodeId& node_id) 
			-> std::tuple<neuria::network::HostName, neuria::network::PortNumber>{
	std::vector<std::string> result_str_list;
	auto node_id_str = node_id.ToString();
	boost::algorithm::split(
		result_str_list, node_id_str, boost::is_any_of(":"));
	if(result_str_list.size() != 2){
		return std::make_tuple(
			neuria::network::HostName(""), neuria::network::PortNumber(""));
	}
	const auto host_name = neuria::network::HostName(result_str_list.at(0));
	const auto port_num = neuria::network::PortNumber(result_str_list.at(1));
	return std::make_tuple(host_name, port_num);
}

auto OutputUseCount(
		std::ostream& os, const neuria::network::Connection::Ptr connection) -> void {
	os << boost::format("%1%:%2%") 
		% connection.get() 
		% connection.use_count()
	<< std::endl;
}

class CuiSyncia{
public:
    CuiSyncia(const neuria::network::HostName& host_name, 
			const neuria::network::PortNumber& port_num,
			const neuria::network::BufferSize& buffer_size) 
		: node_id(CreateNodeIdFromHostNameAndPortNumber(host_name, port_num)), 
			buffer_size(buffer_size),
			cui_shell(), work(io_service),
			command_dispatcher(
				neuria::command::AsyncExecuter([this](boost::function<void ()> func){
					this->io_service.post(func);
				}),
				neuria::command::OnFailedFunc()
			),
			client(io_service),
			connection_pool(this->io_service),
			download_directory_path(this->io_service, 
					database::FileSystemPath("./download")),
			max_hop_count(6){
			}

private:
	auto CreateLink(
			const neuria::network::HostName& host_name,
			const neuria::network::PortNumber& port_number) -> void {
		std::cout << boost::format("ConnectTo\"%1%:%2%\"") 
			% host_name% port_number
		<< std::endl;
		auto on_connected = neuria::network::OnConnectedFunc(
				[this](neuria::network::Socket::Ptr socket){
			std::cout << "Connected!" << std::endl;	
			const auto connection = neuria::network::Connection::Create(
				socket, this->buffer_size
			);
			OutputUseCount(std::cout << __LINE__ << ":", connection);//2
			std::cout << "Added!!" << std::endl;
			this->connection_pool.Add(connection);
			OutputUseCount(std::cout << __LINE__ << ":", connection);//3
			this->StartReceive(connection);
			OutputUseCount(std::cout << __LINE__ << ":", connection);//5
		}); 
		this->client.Connect(host_name, port_number, 
			on_connected,
			neuria::network::OnFailedFunc()
		);	
		
	}

	auto StartReceive(const neuria::network::Connection::Ptr& connection) -> void {
		connection->StartReceive(
			neuria::network::OnReceivedFunc([this, connection](
					const neuria::network::ByteArray& byte_array){
				this->command_dispatcher.Dispatch(
					neuria::command::ByteArraySender([connection](
							const neuria::command::ByteArray& byte_array, 
							const neuria::command::OnSendedFunc& on_sended, 
							const neuria::command::OnFailedFunc& on_failed){
						connection->Send(byte_array, 
							neuria::network::OnSendedFunc(on_sended), 
							neuria::network::OnFailedFunc(/*todo!! on_failed*/)
						);	
					}),
					neuria::command::ConnectionCloser([connection]{
						connection->Close();
						std::cout << "connection closer called" << std::endl;
					}),
					byte_array
				);
			}),
			neuria::network::Connection::OnPeerClosedFunc(
					[this](const neuria::network::Connection::Ptr& connection){
				std::cout << "peer closed" << std::endl;
				OutputUseCount(std::cout << __LINE__ << ":", connection);//8
				this->connection_pool.Remove(connection);
				OutputUseCount(std::cout << __LINE__ << ":", connection);//7
			}),
			neuria::network::OnFailedFunc()	
		);	
	}

public:
	auto InitShell() -> void {
		neuria::test::RegisterExitFunc(this->cui_shell, "bye :)");		

		this->cui_shell.Register("setdd", 
			"<directory_name> set download directory path.", 
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				std::ifstream ifs(arg_list.at(1));
				if(!ifs){
					std::cout << "directory not found" <<std::endl;
					return;
				}
				this->download_directory_path.Assign(
					database::FileSystemPath(arg_list.at(1)));
			})
		);
		
		this->cui_shell.Register("getdd", 
			"get download directory path.", 
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				this->download_directory_path.Quote([](const database::FileSystemPath& download_directory_path){
					std::cout << download_directory_path << std::endl;	
				});
			})
		);

		this->cui_shell.Register("upload", "<file_name> add file to upload list.", 
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				std::ifstream ifs(arg_list.at(1));
				if(!ifs){
					std::cout << "file not found" <<std::endl;
					return;
				}
				this->file_key_hash_db.Add(
					database::FileKeyHash(
						this->node_id, 
						database::FileSystemPath(arg_list.at(1)))
				);
			})
		);

		this->cui_shell.Register("db", "show upload db.", 
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				std::cout << this->file_key_hash_db << std::endl;
			})
		);
		

		this->cui_shell.Register("link", "<host_name> <port_num> create link.", 
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				this->CreateLink(
					neuria::network::HostName(arg_list.at(1)),
					neuria::network::PortNumber(
						boost::lexical_cast<int>(arg_list.at(2)))
				);
			})
		);
		
		this->cui_shell.Register("pool", "show connection pool",
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				std::cout << this->connection_pool << std::endl;	
			})
		);

		this->cui_shell.Register("close", "<host_name> <port_number> close connection",
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				this->connection_pool.PickUpAndQuote(
					neuria::network::HostName(arg_list.at(1)),
					neuria::network::PortNumber(arg_list.at(2)),
					[](const neuria::network::Connection::Ptr& connection){
						connection->Close();
					}
				);
			})
		);
		
		this->cui_shell.Register("echo", "<text> request echo text",
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				auto wrapper = neuria::command::DispatchCommandWrapper(
					command::EchoCommand::GetRequestCommandId(), 
					command::EchoCommand(arg_list.at(1)).Serialize()
				);
				connection_pool.ForEach([wrapper](
						const neuria::network::Connection::Ptr& connection){
					connection->Send(
						wrapper.Serialize(),
						neuria::network::OnSendedFunc(),
						neuria::network::OnFailedFunc()
					);
				});
			})
		);
		
		this->cui_shell.Register("bigecho", "<byte_size> request echo big generated text",
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				const neuria::command::ByteArray big_byte_array(
					boost::lexical_cast<unsigned int>(arg_list.at(1)), 'a');
				auto wrapper = neuria::command::DispatchCommandWrapper(
					command::EchoCommand::GetRequestCommandId(), 
					command::EchoCommand(
						neuria::command::CreateStringFromByteArray(big_byte_array))
							.Serialize()
				);
				connection_pool.ForEach([wrapper](
						const neuria::network::Connection::Ptr& connection){
					connection->Send(
						wrapper.Serialize(),
						neuria::network::OnSendedFunc(),
						neuria::network::OnFailedFunc()
					);
				});
			})
		);
		
		this->cui_shell.Register("pull", "<index> send pull request",
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				const auto file_key_hash_index = 
					boost::lexical_cast<unsigned int>(arg_list.at(1));
				const auto file_key_hash = 
					this->file_key_hash_db.GetByIndex(file_key_hash_index);
				const auto file_command = 
					command::FileCommand(file_key_hash.GetHashId().Serialize());
				std::cout << file_command << std::endl;
				auto wrapper = neuria::command::DispatchCommandWrapper(
					syncia::command::FileCommand::GetPullCommandId(), 
					file_command.Serialize()
				);
				const auto host_and_port_tuple = 
					CreateHostNameAndPortNumberFromNodeId(file_key_hash.GetOwnerId());

				auto on_connected = neuria::network::OnConnectedFunc(
						[this, wrapper](neuria::network::Socket::Ptr socket){
					std::cout << "Connected!" << std::endl;	
					const auto connection = neuria::network::Connection::Create(
						socket, this->buffer_size
					);
					this->connection_pool.Add(connection);
					this->StartReceive(connection);
					connection->Send(wrapper.Serialize(), 
						neuria::network::OnSendedFunc([connection](){
							//connection->Close();	
						}),
						neuria::network::OnFailedFunc()
					);

				}); 
				this->client.Connect(
					std::get<0>(host_and_port_tuple), 
					std::get<1>(host_and_port_tuple), 
					on_connected,
					neuria::network::OnFailedFunc()
				);	
			})
		);
		
		this->cui_shell.Register("mpull", 
			"<host_name> <port_num> <hash_id> send pull request",
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				const auto file_command = 
					command::FileCommand(database::HashId(arg_list.at(3)).Serialize());
				std::cout << file_command << std::endl;
				auto wrapper = neuria::command::DispatchCommandWrapper(
					syncia::command::FileCommand::GetPullCommandId(), 
					file_command.Serialize()
				);
				
				auto on_connected = neuria::network::OnConnectedFunc(
						[this, wrapper](neuria::network::Socket::Ptr socket){
					std::cout << "Connected!" << std::endl;	
					const auto connection = neuria::network::Connection::Create(
						socket, this->buffer_size
					);
					this->StartReceive(connection);
					connection->Send(wrapper.Serialize(), 
						neuria::network::OnSendedFunc([connection](){
							//connection->Close();	
						}),
						neuria::network::OnFailedFunc()
					);

				}); 
				this->client.Connect(
					neuria::network::HostName(arg_list.at(1)),
					neuria::network::PortNumber(arg_list.at(2)),
					on_connected,
					neuria::network::OnFailedFunc()
				);	
			})
		);

		this->cui_shell.Register("search", "[<keyword>] search file key hash",
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				std::string keyword_str;
				for(unsigned int i = 1; i < arg_list.size(); ++i){
					keyword_str = keyword_str + arg_list.at(i) + " ";
				}
				const auto file_key_hash_command = 
					command::FileKeyHashCommand(
						command::NodeId(this->node_id.ToString()),
						command::Keyword(keyword_str));
				std::cout << file_key_hash_command << std::endl;
				auto wrapper = neuria::command::DispatchCommandWrapper(
					syncia::command::FileKeyHashCommand::GetFetchPullCommandId(), 
					file_key_hash_command.Serialize()
				);
				
				this->connection_pool.QuoteRandomConnection([wrapper](
						const neuria::network::Connection::Ptr& connection){
					connection->Send(
						wrapper.Serialize(),
						neuria::network::OnSendedFunc(),
						neuria::network::OnFailedFunc()
					);
				});
			})
		);

		/*
		this->cui_shell.Register("check", 
				"<directory_path> check directory change",
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				
			})
		);
		*/
	}

	auto InitDispatcher() -> void {
		this->command_dispatcher.RegisterFunc(
			syncia::command::EchoCommand::GetRequestCommandId(),
			neuria::command::OnReceivedFunc([](
					const neuria::command::ByteArraySender& sender, 
					const neuria::command::ConnectionCloser& closer,
					const neuria::command::ByteArray& received_byte_array){
				const auto echo_command = command::EchoCommand::Parse(received_byte_array);
				std::cout << boost::format("requested echo text:\"%1%\"")
					% echo_command.GetText()
				<< std::endl;
				const auto dispatch_command = 
					neuria::command::DispatchCommandWrapper(
						command::EchoCommand::GetReplyCommandId(),
						echo_command.Serialize());
				sender(
					dispatch_command.Serialize(),
					neuria::command::OnSendedFunc(),
					neuria::command::OnFailedFunc()
				);
			})
		);
		
		this->command_dispatcher.RegisterFunc(
			syncia::command::EchoCommand::GetReplyCommandId(),
			neuria::command::OnReceivedFunc([](
					const neuria::command::ByteArraySender& sender, 
					const neuria::command::ConnectionCloser& closer,
					const neuria::command::ByteArray& received_byte_array){
				const auto command = 
					command::EchoCommand::Parse(received_byte_array);
				std::cout << boost::format("replied echo text:\"%1%\"")
					% command.GetText()
				<< std::endl;
			})
		);

		this->command_dispatcher.RegisterFunc(
			syncia::command::FileCommand::GetPullCommandId(),
			neuria::command::OnReceivedFunc([this](
					const neuria::command::ByteArraySender& sender, 
					const neuria::command::ConnectionCloser& closer,
					const neuria::command::ByteArray& received_byte_array){
				auto command = 
					command::FileCommand::Parse(received_byte_array);
				const auto hash_id = 
					database::HashId::Parse(command.GetHashIdByteArray());
				std::cout << boost::format("file pull request received:\"%1%\"")
					% hash_id.ToString()
				<< std::endl;
				
				const auto file_key_hash = this->file_key_hash_db.GetByHashId(hash_id);
				std::cout << "requested:" << file_key_hash << std::endl;
				command.SetFileBodyByteArray(
					database::SerializeFile(file_key_hash.GetFilePath()));
				const auto wrapper = neuria::command::DispatchCommandWrapper(
						syncia::command::FileCommand::GetPushCommandId(),
						command.Serialize()
					);
				sender(wrapper.Serialize(), 
					neuria::command::OnSendedFunc(), 
					neuria::command::OnFailedFunc()
				);
			})
		);
		
		this->command_dispatcher.RegisterFunc(
			syncia::command::FileCommand::GetPushCommandId(),
			neuria::command::OnReceivedFunc([this](
					const neuria::command::ByteArraySender& sender, 
					const neuria::command::ConnectionCloser& closer,
					const neuria::command::ByteArray& received_byte_array){
				auto command = 
					command::FileCommand::Parse(received_byte_array);
				const auto hash_id = 
					database::HashId::Parse(command.GetHashIdByteArray());
				
				//todo mpull$B$G%(%i$k(B
				/*
				const auto file_key_hash = this->file_key_hash_db.GetByHashId(hash_id);
				std::cout << boost::format("file push request received:\"%1%\"")
					% file_key_hash
				<< std::endl;
				*/
				this->download_directory_path.Quote([/*file_key_hash,*/ hash_id, command, closer](
						const database::FileSystemPath& download_directory_path){
					std::cout << "quote called." << std::endl;
					database::ParseFile(
						download_directory_path,
						/*file_key_hash.GetFilePath().filename()*/
						database::FileSystemPath(hash_id.ToString()),
						command.GetFileBodyByteArray()
					);	
					closer();
				});
			})
		);

		this->command_dispatcher.RegisterFunc(
			syncia::command::FileKeyHashCommand::GetFetchPullCommandId(),
			neuria::command::OnReceivedFunc([this](
					const neuria::command::ByteArraySender& sender, 
					const neuria::command::ConnectionCloser& closer,
					const neuria::command::ByteArray& received_byte_array){
				auto file_key_hash_command = 
					command::FileKeyHashCommand::Parse(received_byte_array);
				
				file_key_hash_command.AddNodeIdOnRoute(
					command::NodeId(this->node_id.ToString()));
				if(this->max_hop_count < file_key_hash_command.GetCurrentHopCount()){
					if(file_key_hash_command.GetInitialSenderNodeId() == 
							command::NodeId(this->node_id.ToString())){
						std::cout << "FetchPush returned initial sender!" << std::endl;
						return;
					}
					const auto on_connected = neuria::network::OnConnectedFunc(
							[this, file_key_hash_command](
								neuria::network::Socket::Ptr socket){
						const auto fetch_push_wrapper = 
							neuria::command::DispatchCommandWrapper(
								syncia::command::FileKeyHashCommand::GetFetchPushCommandId(), 
								file_key_hash_command.Serialize());
						std::cout << "Connected!" << std::endl;	
						const auto connection = neuria::network::Connection::Create(
							socket, this->buffer_size
						);
						std::cout << "Added!!" << std::endl;
						this->connection_pool.Add(connection);
						this->StartReceive(connection);
						connection->Send(
							fetch_push_wrapper.Serialize(),
							neuria::network::OnSendedFunc(),
							neuria::network::OnFailedFunc()
						);

					}); 
					const auto next_node_id = file_key_hash_command.GetNextPushNodeId(
						command::NodeId(this->node_id.ToString()));
					const auto host_port_tuple = 
						CreateHostNameAndPortNumberFromNodeId(
							database::NodeId(next_node_id.ToString()));
					this->client.Connect(
						std::get<0>(host_port_tuple), 
						std::get<1>(host_port_tuple),  
						on_connected,
						neuria::network::OnFailedFunc()
					);	
				}
				
				const auto fetch_pull_wrapper = neuria::command::DispatchCommandWrapper(
					syncia::command::FileKeyHashCommand::GetFetchPullCommandId(), 
					file_key_hash_command.Serialize()
				);
				this->connection_pool.QuoteRandomConnection([fetch_pull_wrapper](
						const neuria::network::Connection::Ptr& connection){
					connection->Send(
						fetch_pull_wrapper.Serialize(),
						neuria::network::OnSendedFunc(),
						neuria::network::OnFailedFunc()
					);
				});
				
			})
		);
		
		this->command_dispatcher.RegisterFunc(
			syncia::command::FileKeyHashCommand::GetFetchPushCommandId(),
			neuria::command::OnReceivedFunc([this](
					const neuria::command::ByteArraySender& sender, 
					const neuria::command::ConnectionCloser& closer,
					const neuria::command::ByteArray& received_byte_array){
				auto file_key_hash_command = 
					command::FileKeyHashCommand::Parse(received_byte_array);
				
				file_key_hash_command.AddNodeIdOnRoute(
					command::NodeId(this->node_id.ToString()));
				if(file_key_hash_command.GetInitialSenderNodeId() == 
						command::NodeId(this->node_id.ToString())){
					std::cout << "FetchPush returned initial sender!" << std::endl;
					return;
				}
				
				const auto on_connected = neuria::network::OnConnectedFunc(
						[this, file_key_hash_command](neuria::network::Socket::Ptr socket){
					const auto fetch_push_wrapper = 
						neuria::command::DispatchCommandWrapper(
							syncia::command::FileKeyHashCommand::GetFetchPushCommandId(), 
							file_key_hash_command.Serialize());
					std::cout << "Connected!" << std::endl;	
					const auto connection = neuria::network::Connection::Create(
						socket, this->buffer_size
					);
					std::cout << "Added!!" << std::endl;
					this->connection_pool.Add(connection);
					this->StartReceive(connection);
					connection->Send(
						fetch_push_wrapper.Serialize(),
						neuria::network::OnSendedFunc(),
						neuria::network::OnFailedFunc()
					);

				}); 
				const auto next_node_id = file_key_hash_command.GetNextPushNodeId(
					command::NodeId(this->node_id.ToString()));
				const auto host_port_tuple = 
					CreateHostNameAndPortNumberFromNodeId(
						database::NodeId(next_node_id.ToString()));
				this->client.Connect(
					std::get<0>(host_port_tuple), 
					std::get<1>(host_port_tuple),  
					on_connected,
					neuria::network::OnFailedFunc()
				);	
			})
		);
	}

	auto Run() -> void {
		boost::thread_group thread_group;
		for(unsigned int i = 0; i < 5; ++i){
			thread_group.create_thread(
				boost::bind(&boost::asio::io_service::run, &this->io_service)
			);
		}
		std::cout << boost::format("ServerReady:\"%1%\"") 
			% this->node_id.ToString()
		<< std::endl;
		
		const auto host_port_tuple = CreateHostNameAndPortNumberFromNodeId(this->node_id); 

		neuria::network::Server server(this->io_service, std::get<1>(host_port_tuple),
			neuria::network::OnAcceptedFunc(
					[this](neuria::network::Socket::Ptr socket){
				std::cout << "accepted!" << std::endl;
				auto connection = 
					neuria::network::Connection::Create(socket, buffer_size);
				this->connection_pool.Add(connection);
				this->StartReceive(connection);
			}),
			neuria::network::OnFailedFunc()
		);
		server.StartAccept();
		for(unsigned int i = 0; i < 400; ++i){
			//this->cui_shell.Call("link localhost 54321");
			//this->cui_shell.Call("close 0");
		}
		/*
		for(unsigned int i = 0; i < 500; ++i){
			this->cui_shell.Call("echo helllo");
		}
		*/
		this->cui_shell.Call("setdd ./download");
		cui_shell.Start();
		thread_group.join_all();
	}

private:
	const database::NodeId node_id;
	const neuria::network::BufferSize buffer_size;
	neuria::test::CuiShell cui_shell;
	boost::asio::io_service io_service;
	boost::asio::io_service::work work;
	neuria::command::CommandDispatcher command_dispatcher;
	neuria::network::Client client;
	neuria::network::ConnectionPool connection_pool;//<-thread safe ;)
	database::FileKeyHashDb file_key_hash_db;//need strand
	neuria::thread_safe::ThreadSafeVariable<database::FileSystemPath> download_directory_path;
	const command::HopCount max_hop_count;
};
}
