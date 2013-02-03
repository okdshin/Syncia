#pragma once
//CuiSyncia:20130125
#include <iostream>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include "Neuria/Neuria.h"
#include "command/EchoCommand.h"
#include "command/FileCommand.h"
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

class CuiSyncia{
public:
    CuiSyncia(const neuria::network::HostName& host_name, 
			const neuria::network::PortNumber& port_num,
			const neuria::network::BufferSize& buffer_size) 
		: host_name(host_name), port_num(port_num), buffer_size(buffer_size),
			cui_shell(), io_service(neuria::network::IoService::Create()), 
			work(io_service->GetRawIoServiceRef()),
			command_dispatcher(
				neuria::command::AsyncExecuter([this](boost::function<void ()> func){
					this->io_service->GetRawIoServiceRef().post(func);
				}),
				neuria::command::OnFailedFunc()
			),
			client(io_service),
			download_directory_path(database::FileSystemPath("./")){}

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
			std::cout << "Added!!" << std::endl;
			this->connection_pool.Add(connection);
			this->StartReceive(connection);
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
					byte_array
				);
			}),
			neuria::network::Connection::OnPeerClosedFunc(
					[this](const neuria::network::Connection::Ptr& connection){
				std::cout << "peer closed" << std::endl;
				this->connection_pool.Remove(connection);
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
				this->SetDownloadDirectoryPath( 
					database::FileSystemPath(arg_list.at(1)));
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
						CreateNodeIdFromHostNameAndPortNumber(
							this->host_name, this->port_num), 
						database::FileSystemPath(arg_list.at(1)))
				);
			})
		);

		this->cui_shell.Register("list", "show upload list.", 
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

		this->cui_shell.Register("close", "<connection_index> close connection",
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				const auto index = 
					boost::lexical_cast<unsigned int>(arg_list.at(1));
				this->connection_pool.At(index)->Close();
			})
		);

		this->cui_shell.Register("echo", "<text> request echo text",
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				auto wrapper = neuria::command::DispatchCommandWrapper(
					command::EchoCommand::GetRequestCommandId(), 
					command::EchoCommand(arg_list.at(1)).Serialize()
				);
				connection_pool.ForEach([&wrapper](
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
		
		/*
		this->cui_shell.Register("echon", 
				"<number> <text> request echo text <number> times",
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				this->cui_shell.Call(boost::format("echo %1%") %);
			})
		);
		*/
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
					const neuria::command::ByteArray& received_byte_array){
				auto command = 
					command::FileCommand::Parse(received_byte_array);
				const auto hash_id = 
					database::HashId::Parse(command.GetHashIdByteArray());
				const auto file_key_hash = this->file_key_hash_db.GetByHashId(hash_id);
				std::cout << boost::format("file push request received:\"%1%\"")
					% file_key_hash
				<< std::endl;
				database::ParseFile(
					this->download_directory_path,
					file_key_hash.GetFilePath(),
					command.GetFileBodyByteArray()
				);
			})
		);
	}

	auto SetDownloadDirectoryPath(
			const database::FileSystemPath& download_directory_path) -> void {
		this->download_directory_path = download_directory_path;
	}

	auto Run() -> void {
		boost::thread_group thread_group;
		for(unsigned int i = 0; i < 5; ++i){
			thread_group.create_thread(
				boost::bind(&boost::asio::io_service::run, 
					&io_service->GetRawIoServiceRef())
			);
		}
		std::cout << boost::format("ServerReady:\"%1%:%2%\"") 
			% host_name.ToString() % port_num.ToInt()
		<< std::endl;
		neuria::network::Server server(this->io_service, this->port_num,
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
		for(unsigned int i = 0; i < 1/*500*/; ++i){
			//this->cui_shell.Call("link wirelessia.net 20550");
		}
		cui_shell.Start();
		thread_group.join_all();
	}

private:
	neuria::network::HostName host_name;
	neuria::network::PortNumber port_num;
	neuria::network::BufferSize buffer_size;
	neuria::test::CuiShell cui_shell;
	neuria::network::IoService::Ptr io_service;
	boost::asio::io_service::work work;
	neuria::command::CommandDispatcher command_dispatcher;
	neuria::network::Client client;
	neuria::network::ConnectionPool connection_pool;
	database::FileKeyHashDb file_key_hash_db;
	database::FileSystemPath download_directory_path;
};
}
