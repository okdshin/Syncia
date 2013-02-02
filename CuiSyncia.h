#pragma once
//CuiSyncia:20130125
#include <iostream>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include "Neuria/Neuria.h"
#include "command/EchoCommand.h"
namespace syncia
{
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
			client(io_service){}

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
			auto connection = neuria::network::Connection::Ptr();
			connection = neuria::network::Connection::Create(
				socket, this->buffer_size
			);
			std::cout << "Added!!" << std::endl;
			this->connection_pool.Add(connection);
			connection->StartReceive(
				neuria::network::OnReceivedFunc([this, connection](
						const neuria::network::Socket::Ptr& socket,
						const neuria::network::ByteArray& byte_array){
					this->command_dispatcher.Dispatch(
						neuria::command::ByteArraySender([connection](
								const neuria::command::ByteArray& byte_array, 
								const neuria::command::OnSendedFunc& on_sended, 
								const neuria::command::OnFailedFunc& on_failed){
							connection->Send(byte_array, 
								neuria::network::OnSendedFunc(on_sended), 
								neuria::network::OnFailedFunc(/*on_failed*/)
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
		}); 
		this->client.Connect(host_name, port_number, 
			on_connected,
			neuria::network::OnFailedFunc()
		);	
		
	}

public:

	auto InitShell() -> void {
		neuria::test::RegisterExitFunc(this->cui_shell, "bye :)");		

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

		this->cui_shell.Register("echo", "<text> echo text",
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				auto wrapper = neuria::command::DispatchCommandWrapper(
					syncia::command::EchoCommand::GetRequestCommandId(), 
					neuria::command::CreateByteArrayFromString(arg_list.at(1))
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
	}

	auto InitDispatcher() -> void {
		this->command_dispatcher.RegisterFunc(
			syncia::command::EchoCommand::GetRequestCommandId(),
			neuria::command::OnReceivedFunc([](
					const neuria::command::ByteArraySender& sender, 
					const neuria::command::ByteArray& received_byte_array){
				std::cout << boost::format("received echo text:\"%1%\"")
					% neuria::network::CreateStringFromByteArray(received_byte_array) 
				<< std::endl;
				const auto dispatch_command = 
					neuria::command::DispatchCommandWrapper(
						command::EchoCommand::GetReplyCommandId(),
						command::EchoCommand(received_byte_array).Serialize());
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
				std::cout << boost::format("received echo text:\"%1%\"")
					% neuria::command::CreateStringFromByteArray(
						command.GetWrappedByteArray())
				<< std::endl;
			})
		);
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
				/*
				connection->StartReceive(
					neuria::network::OnReceivedFunc([this, connection](
							const neuria::network::Socket::Ptr& socket,
							const neuria::network::ByteArray& byte_array){
						this->command_dispatcher.Dispatch(
							neuria::command::ByteArraySender([connection](
									const neuria::command::ByteArray& byte_array, 
									const neuria::command::OnSendedFunc& on_sended, 
									const neuria::command::OnFailedFunc& on_failed){
								connection->Send(byte_array, 
									neuria::network::OnSendedFunc(on_sended), 
									neuria::network::OnFailedFunc(on_failed));	
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
				*/
				connection->StartReceive(
					neuria::network::OnReceivedFunc([this, connection](
							const neuria::network::Socket::Ptr& socket,
							const neuria::network::ByteArray& byte_array){
						this->command_dispatcher.Dispatch(
							neuria::command::ByteArraySender([connection](
									const neuria::command::ByteArray& byte_array, 
									const neuria::command::OnSendedFunc& on_sended, 
									const neuria::command::OnFailedFunc& on_failed){
								connection->Send(byte_array, 
									neuria::network::OnSendedFunc(on_sended), 
									neuria::network::OnFailedFunc(/*on_failed*/)
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
};
}
