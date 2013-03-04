#pragma once
//CuiSyncia:20130125
#include <iostream>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include "Neuria/Neuria.h"
#include "Neuria/timer/MultipleTimer.h"
#include "command/EchoCommand.h"
#include "command/FileCommand.h"
#include "command/FileKeyHashCommand.h"
#include "command/HopCount.h"
#include "database/FileKeyHashDb.h"
#include "database/FileSystemPath.h"
#include "SearchFileKeyHashDb.h"
#include "SpreadFileKeyHashDb.h"

namespace syncia
{
inline auto CreateNodeIdFromHostNameAndPortNumber(
		const neuria::network::HostName& host_name,
		const neuria::network::PortNumber& port_num) -> database::NodeId {
	return database::NodeId(host_name.ToString()+":"+port_num.ToString());
}

inline auto CreateHostNameAndPortNumberFromNodeId(
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

inline auto OutputUseCount(
		std::ostream& os, const neuria::network::Connection::Ptr connection) -> void {
	os << boost::format("%1%:%2%") 
		% connection.get() 
		% connection.use_count()
	<< std::endl;
}

class CuiSyncia{
public:
    CuiSyncia(boost::asio::io_service& io_service,
			const neuria::network::HostName& host_name, 
			const neuria::network::PortNumber& port_num,
			const neuria::network::BufferSize& buffer_size,
			const database::FileSystemPath& download_directory_path,
			const SearchFileKeyHashDb& search_file_key_hash_db,
			const SpreadFileKeyHashDb& spread_file_key_hash_db,
			std::ostream& front_os, 
			std::ostream& log_os,
			std::istream& is) 
		: front_os(front_os), log_os(log_os), is(is), io_service(io_service), work(io_service),
			node_id(CreateNodeIdFromHostNameAndPortNumber(host_name, port_num)), 
			buffer_size(buffer_size),
			server(io_service, port_num,
				neuria::network::OnAcceptedFunc(
						[this](neuria::network::Socket::Ptr socket){
					this->log_os << "accepted!" << std::endl;
					auto connection = 
						neuria::network::Connection::Create(socket, this->buffer_size);
					this->connection_pool.Add(connection);
					this->StartReceive(connection);
				}),
				neuria::network::OnFailedFunc()
			),
			cui_shell(this->front_os, is),
			command_dispatcher(
				neuria::command::AsyncExecuter([this](boost::function<void ()> func){
					this->io_service.post(func);
				}),
				neuria::command::OnFailedFunc()
			),
			client(io_service),
			connection_pool(io_service),
			search_file_key_hash_db(search_file_key_hash_db()),
			spread_file_key_hash_db(spread_file_key_hash_db()),
			download_directory_path(io_service, download_directory_path),
			max_hop_count(6),
			spread_max_count(100),
			multiple_timer(io_service),
			spread_file_key_hash_interval(30),
			check_upload_directory_interval(10){
			}

private:
	auto CreateLink(
			const neuria::network::HostName& host_name,
			const neuria::network::PortNumber& port_number) -> void {
		this->log_os << boost::format("ConnectTo\"%1%:%2%\"") 
			% host_name% port_number
		<< std::endl;
		auto on_connected = neuria::network::OnConnectedFunc(
				[this](neuria::network::Socket::Ptr socket){
			this->log_os << "Connected!" << std::endl;	
			const auto connection = neuria::network::Connection::Create(
				socket, this->buffer_size
			);
			this->log_os << "Added!!" << std::endl;
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
					neuria::command::ConnectionCloser([this, connection]{
						connection->Close();
						this->log_os << "connection closer called" << std::endl;
					}),
					byte_array
				);
			}),
			neuria::network::Connection::OnPeerClosedFunc(
					[this](const neuria::network::Connection::Ptr& connection){
				this->log_os << "peer closed" << std::endl;
				this->connection_pool.Remove(connection);
			}),
			neuria::network::OnFailedFunc()	
		);	
	}

public:
	auto InitShell() -> void {
		neuria::shell::RegisterExitFunc(this->cui_shell, "bye :)");		

		this->cui_shell.Register("setdd", 
			"<directory_name> set download directory path.", 
			neuria::shell::ShellFunc(
					[this](const neuria::shell::CuiShell::ArgList& arg_list){
				std::ifstream ifs(arg_list.at(1));
				if(!ifs){
					this->front_os << "directory not found" <<std::endl;
					return;
				}
				this->download_directory_path.Assign(
					database::FileSystemPath(arg_list.at(1)));
			})
		);
		
		this->cui_shell.Register("getdd", 
			"get download directory path.", 
			neuria::shell::ShellFunc(
					[this](const neuria::shell::CuiShell::ArgList& arg_list){
				this->download_directory_path.Quote(
						[this](
							const database::FileSystemPath& download_directory_path){
					this->front_os << download_directory_path << std::endl;	
				});
			})
		);

		this->cui_shell.Register("upload", "<file_name> add file to upload list.", 
			neuria::shell::ShellFunc(
					[this](const neuria::shell::CuiShell::ArgList& arg_list){
				std::ifstream ifs(arg_list.at(1));
				if(!ifs){
					this->front_os << "file not found" <<std::endl;
					return;
				}
				this->spread_file_key_hash_db->Add(
					database::FileKeyHash(
						this->node_id, 
						database::FileSystemPath(arg_list.at(1)))
				);
			})
		);

		this->cui_shell.Register("db", "show found file key has list.", 
			neuria::shell::ShellFunc(
					[this](const neuria::shell::CuiShell::ArgList& arg_list){
				this->front_os << *(this->search_file_key_hash_db) << std::endl;
			})
		);
		
		this->cui_shell.Register("spdb", "show upload db.", 
			neuria::shell::ShellFunc(
					[this](const neuria::shell::CuiShell::ArgList& arg_list){
				this->front_os << *(this->spread_file_key_hash_db) << std::endl;
			})
		);
		

		this->cui_shell.Register("link", "<host_name> <port_num> create link.", 
			neuria::shell::ShellFunc(
					[this](const neuria::shell::CuiShell::ArgList& arg_list){
				this->CreateLink(
					neuria::network::HostName(arg_list.at(1)),
					neuria::network::PortNumber(
						boost::lexical_cast<int>(arg_list.at(2)))
				);
			})
		);
		
		this->cui_shell.Register("pool", "show connection pool",
			neuria::shell::ShellFunc(
					[this](const neuria::shell::CuiShell::ArgList& arg_list){
				this->front_os << this->connection_pool << std::endl;	
			})
		);

		this->cui_shell.Register("close", "<host_name> <port_number> close connection",
			neuria::shell::ShellFunc(
					[this](const neuria::shell::CuiShell::ArgList& arg_list){
				this->connection_pool.PickUpAndQuote(
					[arg_list](const neuria::network::Connection::Ptr& connection) -> bool{
						return connection->GetRemoteHostName() == 
							neuria::network::HostName(arg_list.at(1)) && 
							connection->GetRemotePortNumber() == 
							neuria::network::PortNumber(arg_list.at(2));
							
					},
					[](const neuria::network::ConnectionList& connection_list){
						for(const auto& connection : connection_list){
							connection->Close();
						}
					}
				);
			})
		);
		
		this->cui_shell.Register("echo", "<text> request echo text",
			neuria::shell::ShellFunc(
					[this](const neuria::shell::CuiShell::ArgList& arg_list){
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
			neuria::shell::ShellFunc(
					[this](const neuria::shell::CuiShell::ArgList& arg_list){
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
		
		
		this->cui_shell.Register("pull", "<hash_id> send pull request",
			neuria::shell::ShellFunc(
					[this](const neuria::shell::CuiShell::ArgList& arg_list){
				const auto hash_id = database::HashId(arg_list.at(1));
				this->search_file_key_hash_db->QuoteByHashId(hash_id,
					[this](const database::FileKeyHashList& file_key_hash_list){	
						if(file_key_hash_list.empty()){
							this->front_os << "(pull)hash id not found." << std::endl;
							return;
						}
						const auto file_key_hash = file_key_hash_list.front();
						const auto file_command = 
							command::FileCommand(file_key_hash.Serialize());
						this->log_os << file_command << std::endl;
						auto wrapper = neuria::command::DispatchCommandWrapper(
							syncia::command::FileCommand::GetPullCommandId(), 
							file_command.Serialize()
						);
						const auto host_and_port_tuple = 
							CreateHostNameAndPortNumberFromNodeId(
								file_key_hash.GetOwnerId());
						
						auto on_connected = neuria::network::OnConnectedFunc(
								[this, wrapper](neuria::network::Socket::Ptr socket){
							this->log_os << "Connected!" << std::endl;	
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
					}
				);
			})
			
		);
		
		/*
		this->cui_shell.Register("mpull", 
			"<host_name> <port_num> <hash_id> <save_file_path> send pull request",
			neuria::shell::ShellFunc(
					[this](const neuria::shell::CuiShell::ArgList& arg_list){
				const auto file_command = 
					command::FileCommand(database::HashId(arg_list.at(3)).Serialize());
				this->os << file_command << std::endl;
				auto wrapper = neuria::command::DispatchCommandWrapper(
					syncia::command::FileCommand::GetPullCommandId(), 
					file_command.Serialize()
				);
				
				auto on_connected = neuria::network::OnConnectedFunc(
						[this, wrapper](neuria::network::Socket::Ptr socket){
					this->os << "Connected!" << std::endl;	
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
		*/
		this->cui_shell.Register("search", "[<keyword>] search file key hash",
			neuria::shell::ShellFunc(
					[this](const neuria::shell::CuiShell::ArgList& arg_list){
				std::string keyword_str;
				for(unsigned int i = 1; i < arg_list.size(); ++i){
					keyword_str = keyword_str + arg_list.at(i) + " ";
				}
				const auto file_key_hash_command = 
					command::FileKeyHashCommand(
						command::NodeId(this->node_id.ToString()),
						command::Keyword(keyword_str));
				this->log_os << file_key_hash_command << std::endl;
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
		
		this->cui_shell.Register("spread", "spread file key hash",
			neuria::shell::ShellFunc(
					[this](const neuria::shell::CuiShell::ArgList& arg_list){
				const auto file_key_hash_command = 
					command::FileKeyHashCommand(
						command::NodeId(this->node_id.ToString()));
				this->log_os << file_key_hash_command << std::endl;
				auto wrapper = neuria::command::DispatchCommandWrapper(
					syncia::command::FileKeyHashCommand::GetSpreadPullCommandId(), 
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

		this->cui_shell.Register("check", 
				"<directory_path> check directory change",
			neuria::shell::ShellFunc(
					[this](const neuria::shell::CuiShell::ArgList& arg_list){
				auto directory_path = database::FileSystemPath(arg_list.at(1));
				if(!boost::filesystem::is_directory(directory_path)){
					this->front_os << directory_path << " is not directory." << std::endl;
					return;
				}
				const auto end_iter = boost::filesystem::recursive_directory_iterator();
				for(boost::filesystem::recursive_directory_iterator iter(directory_path);
						iter != end_iter; ++iter){
					if(boost::filesystem::is_regular_file(iter->path())){
						boost::format format("upload %1%");
						format % iter->path().string();
						this->cui_shell.Call(format.str());
					}
				}
			})
		);
		
		this->cui_shell.Register("rmold", 
				"remove old file key hash.",
			neuria::shell::ShellFunc(
					[this](const neuria::shell::CuiShell::ArgList& arg_list){
				/*
				this->search_file_key_hash_db->Remove(
						[this](const database::FileKeyHash& file_key_hash) -> bool {
					return file_key_hash.GetLastCheckedTime() 
							+ boost::posix_time::seconds(
								this->check_upload_directory_interval * 2)
						< boost::posix_time::second_clock::universal_time();
				});
				*/
				this->spread_file_key_hash_db->Remove(
						[this](const database::FileKeyHash& file_key_hash) -> bool {
					return file_key_hash.GetLastCheckedTime() 
							+ boost::posix_time::seconds(
								this->check_upload_directory_interval * 2)
						< boost::posix_time::second_clock::universal_time();
				});
			})
		);
	}

	auto InitDispatcher() -> void {
		this->command_dispatcher.RegisterFunc(
			syncia::command::EchoCommand::GetRequestCommandId(),
			neuria::command::OnReceivedFunc([this](
					const neuria::command::ByteArraySender& sender, 
					const neuria::command::ConnectionCloser& closer,
					const neuria::command::ByteArray& received_byte_array){
				const auto echo_command = command::EchoCommand::Parse(received_byte_array);
				this->front_os << boost::format("requested echo text:\"%1%\"")
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
			neuria::command::OnReceivedFunc([this](
					const neuria::command::ByteArraySender& sender, 
					const neuria::command::ConnectionCloser& closer,
					const neuria::command::ByteArray& received_byte_array){
				const auto command = 
					command::EchoCommand::Parse(received_byte_array);
				this->front_os << boost::format("replied echo text:\"%1%\"")
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
				auto file_command = 
					command::FileCommand::Parse(received_byte_array);
				const auto file_key_hash = 
					database::FileKeyHash::Parse(file_command.GetFileKeyHashByteArray());
				this->log_os << boost::format("file pull request received:\"%1%\"")
					% file_key_hash
				<< std::endl;
				
				this->spread_file_key_hash_db->QuoteByHashId(file_key_hash.GetHashId(), 
					[this, file_command, sender](const database::FileKeyHashList& file_key_hash_list){
						const auto file_key_hash = file_key_hash_list.front();
						auto temp_file_command = file_command;
						this->log_os << "requested:" << file_key_hash << std::endl;
						temp_file_command.SetFileBodyByteArray(
							database::SerializeFile(file_key_hash.GetFilePath()));
						const auto wrapper = neuria::command::DispatchCommandWrapper(
								syncia::command::FileCommand::GetPushCommandId(),
								temp_file_command.Serialize()
							);
						sender(wrapper.Serialize(), 
							neuria::command::OnSendedFunc(), 
							neuria::command::OnFailedFunc()
						);	
					}
				);
			})
		);
		
		this->command_dispatcher.RegisterFunc(
			syncia::command::FileCommand::GetPushCommandId(),
			neuria::command::OnReceivedFunc([this](
					const neuria::command::ByteArraySender& sender, 
					const neuria::command::ConnectionCloser& closer,
					const neuria::command::ByteArray& received_byte_array){
				auto file_command = 
					command::FileCommand::Parse(received_byte_array);
				const auto file_key_hash = 
					database::FileKeyHash::Parse(file_command.GetFileKeyHashByteArray());
				
				this->download_directory_path.Quote([this, file_key_hash, file_command, closer](
						const database::FileSystemPath& download_directory_path){
					this->log_os << "quote called." << std::endl;
					const auto file_path = file_key_hash.GetFilePath();

					//check file path
					const bool has_dangerous_dotdot = 
						boost::contains(file_path.string(), "..");
					if(file_path.is_absolute() || has_dangerous_dotdot){
						std::cout << "invalid file path" << std::endl;
						return;
					}

					database::CreateNecessaryDirectory(
						download_directory_path / file_path);
					database::ParseFile(
						download_directory_path / file_path,
						file_command.GetFileBodyByteArray()
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
				this->log_os << file_key_hash_command << std::endl;

				const auto keyword = file_key_hash_command.GetKeyword();
				this->spread_file_key_hash_db->QuoteSearch(
					database::Keyword(keyword.ToString()), 
					[this, file_key_hash_command](
							const database::FileKeyHashList& found_file_key_hash_list){
						auto temp_file_key_hash_command = file_key_hash_command;
						for(const auto& file_key_hash : found_file_key_hash_list){
							temp_file_key_hash_command.AddFileKeyHashByteArray(
								file_key_hash.Serialize());
						}
						//file_key_hash_command.AddFileKeyHash()
						
						temp_file_key_hash_command.AddNodeIdOnRoute(
							command::NodeId(this->node_id.ToString()));
						if(this->max_hop_count < temp_file_key_hash_command.GetCurrentHopCount()){
							if(temp_file_key_hash_command.GetInitialSenderNodeId() == 
									command::NodeId(this->node_id.ToString())){
								temp_file_key_hash_command.ForEach(
										[this](const neuria::command::ByteArray& byte_array){
									const auto file_key_hash = 
										database::FileKeyHash::Parse(byte_array);
									this->log_os << "received!: " << file_key_hash << std::endl;
									this->spread_file_key_hash_db->Add(file_key_hash);
									this->search_file_key_hash_db->Add(file_key_hash);
											
								});
								this->log_os << "FetchPull returned initial sender!" << std::endl;
								return;
							}
							const auto on_connected = neuria::network::OnConnectedFunc(
									[this, temp_file_key_hash_command](
										neuria::network::Socket::Ptr socket){
								const auto fetch_push_wrapper = 
									neuria::command::DispatchCommandWrapper(
										syncia::command::FileKeyHashCommand::GetFetchPushCommandId(), 
										temp_file_key_hash_command.Serialize());
								this->log_os << "Connected!" << std::endl;	
								const auto connection = neuria::network::Connection::Create(
									socket, this->buffer_size
								);
								this->log_os << "Added!!" << std::endl;
								this->connection_pool.Add(connection);
								this->StartReceive(connection);
								connection->Send(
									fetch_push_wrapper.Serialize(),
									neuria::network::OnSendedFunc(),
									neuria::network::OnFailedFunc()
								);

							}); 
							const auto next_node_id = temp_file_key_hash_command.GetNextPushNodeId(
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
							temp_file_key_hash_command.Serialize()
						);
						this->connection_pool.QuoteRandomConnection([fetch_pull_wrapper](
								const neuria::network::Connection::Ptr& connection){
							connection->Send(
								fetch_pull_wrapper.Serialize(),
								neuria::network::OnSendedFunc(),
								neuria::network::OnFailedFunc()
							);
						});
							
					}
				);
				
				
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
				this->log_os << file_key_hash_command << std::endl;
				
				file_key_hash_command.AddNodeIdOnRoute(
					command::NodeId(this->node_id.ToString()));
				if(file_key_hash_command.GetInitialSenderNodeId() == 
						command::NodeId(this->node_id.ToString())){
					file_key_hash_command.ForEach(
							[this](const neuria::command::ByteArray& byte_array){
						this->log_os << "for each ininin" << std::endl;
						const auto file_key_hash = 
							database::FileKeyHash::Parse(byte_array);
						this->log_os << "received!: " << file_key_hash << std::endl;
						this->log_os << "added" << std::endl;
						this->spread_file_key_hash_db->Add(file_key_hash);
						this->search_file_key_hash_db->Add(file_key_hash);
								
					});
					this->log_os << "FetchPush returned initial sender!" << std::endl;
					return;
				}
				
				const auto on_connected = neuria::network::OnConnectedFunc(
						[this, file_key_hash_command](neuria::network::Socket::Ptr socket){
					const auto fetch_push_wrapper = 
						neuria::command::DispatchCommandWrapper(
							syncia::command::FileKeyHashCommand::GetFetchPushCommandId(), 
							file_key_hash_command.Serialize());
					this->log_os << "Connected!" << std::endl;	
					const auto connection = neuria::network::Connection::Create(
						socket, this->buffer_size
					);
					this->log_os << "Added!!" << std::endl;
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

		this->command_dispatcher.RegisterFunc(
			syncia::command::FileKeyHashCommand::GetSpreadPullCommandId(),
			neuria::command::OnReceivedFunc([this](
					const neuria::command::ByteArraySender& sender, 
					const neuria::command::ConnectionCloser& closer,
					const neuria::command::ByteArray& received_byte_array){
				auto file_key_hash_command = 
					command::FileKeyHashCommand::Parse(received_byte_array);
				this->log_os << file_key_hash_command << std::endl;

				this->spread_file_key_hash_db->QuoteNewerList(
					this->spread_max_count, 
					[this, file_key_hash_command](
							const database::FileKeyHashList& found_file_key_hash_list){
						auto temp_file_key_hash_command = file_key_hash_command;
						for(const auto& file_key_hash : found_file_key_hash_list){
							temp_file_key_hash_command.AddFileKeyHashByteArray(
								file_key_hash.Serialize());
						}
						//file_key_hash_command.AddFileKeyHash()
						
						temp_file_key_hash_command.AddNodeIdOnRoute(
							command::NodeId(this->node_id.ToString()));
						if(this->max_hop_count < temp_file_key_hash_command.GetCurrentHopCount()){
							if(temp_file_key_hash_command.GetInitialSenderNodeId() == 
									command::NodeId(this->node_id.ToString())){
								temp_file_key_hash_command.ForEach(
										[this](const neuria::command::ByteArray& byte_array){
									this->log_os << "for each ininin" << std::endl;
									const auto file_key_hash = 
										database::FileKeyHash::Parse(byte_array);
									this->log_os << "received!: " << file_key_hash << std::endl;
									this->log_os << "added" << std::endl;
									this->spread_file_key_hash_db->Add(file_key_hash);
											
								});
								this->log_os << "FetchPull returned initial sender!" << std::endl;
								return;
							}
							const auto on_connected = neuria::network::OnConnectedFunc(
									[this, temp_file_key_hash_command](
										neuria::network::Socket::Ptr socket){
								const auto fetch_push_wrapper = 
									neuria::command::DispatchCommandWrapper(
										syncia::command::FileKeyHashCommand::GetSpreadPushCommandId(), 
										temp_file_key_hash_command.Serialize());
								this->log_os << "Connected!" << std::endl;	
								const auto connection = neuria::network::Connection::Create(
									socket, this->buffer_size
								);
								this->log_os << "Added!!" << std::endl;
								this->connection_pool.Add(connection);
								this->StartReceive(connection);
								connection->Send(
									fetch_push_wrapper.Serialize(),
									neuria::network::OnSendedFunc(),
									neuria::network::OnFailedFunc()
								);

							}); 
							const auto next_node_id = temp_file_key_hash_command.GetNextPushNodeId(
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
							syncia::command::FileKeyHashCommand::GetSpreadPullCommandId(), 
							temp_file_key_hash_command.Serialize()
						);
						this->connection_pool.QuoteRandomConnection([fetch_pull_wrapper](
								const neuria::network::Connection::Ptr& connection){
							connection->Send(
								fetch_pull_wrapper.Serialize(),
								neuria::network::OnSendedFunc(),
								neuria::network::OnFailedFunc()
							);
						});
							
					}
				);
				
				
			})
		);
		
		this->command_dispatcher.RegisterFunc(
			syncia::command::FileKeyHashCommand::GetSpreadPushCommandId(),
			neuria::command::OnReceivedFunc([this](
					const neuria::command::ByteArraySender& sender, 
					const neuria::command::ConnectionCloser& closer,
					const neuria::command::ByteArray& received_byte_array){
				auto file_key_hash_command = 
					command::FileKeyHashCommand::Parse(received_byte_array);
				this->log_os << file_key_hash_command << std::endl;
				
				file_key_hash_command.AddNodeIdOnRoute(
					command::NodeId(this->node_id.ToString()));
				if(file_key_hash_command.GetInitialSenderNodeId() == 
						command::NodeId(this->node_id.ToString())){
					file_key_hash_command.ForEach(
							[this](const neuria::command::ByteArray& byte_array){
						this->log_os << "for each ininin" << std::endl;
						const auto file_key_hash = 
							database::FileKeyHash::Parse(byte_array);
						this->log_os << "received!: " << file_key_hash << std::endl;
						this->log_os << "added" << std::endl;
						this->spread_file_key_hash_db->Add(file_key_hash);
								
					});
					this->log_os << "SpreadPush returned initial sender!" << std::endl;
					return;
				}
				
				const auto on_connected = neuria::network::OnConnectedFunc(
						[this, file_key_hash_command](neuria::network::Socket::Ptr socket){
					const auto fetch_push_wrapper = 
						neuria::command::DispatchCommandWrapper(
							syncia::command::FileKeyHashCommand::GetSpreadPushCommandId(), 
							file_key_hash_command.Serialize());
					this->log_os << "Connected!" << std::endl;	
					const auto connection = neuria::network::Connection::Create(
						socket, this->buffer_size
					);
					this->log_os << "Added!!" << std::endl;
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

	auto InitTimer() -> void {
		this->multiple_timer.AddCallbackFuncAndStartTimer(
			this->spread_file_key_hash_interval,
			neuria::timer::TimerCallbackFunc([this]() -> neuria::timer::IsContinue {
				this->cui_shell.Call("spread");	
				return neuria::timer::IsContinue(true);
			}),
			neuria::timer::OnTimerErasedFunc()
		);	

		this->multiple_timer.AddCallbackFuncAndStartTimer(
			this->check_upload_directory_interval,
			neuria::timer::TimerCallbackFunc([this]() -> neuria::timer::IsContinue {
				this->download_directory_path.Quote(
						[this](const database::FileSystemPath& download_path){
					boost::format format("check %1%");
					format % download_path.string();
					this->cui_shell.Call(format.str());
				});
				return neuria::timer::IsContinue(true);
			}),
			neuria::timer::OnTimerErasedFunc()
		);	
		
		this->multiple_timer.AddCallbackFuncAndStartTimer(
			this->check_upload_directory_interval,
			neuria::timer::TimerCallbackFunc([this]() -> neuria::timer::IsContinue {
				this->cui_shell.Call("rmold");
				return neuria::timer::IsContinue(true);
			}),
			neuria::timer::OnTimerErasedFunc()
		);	
	}

	auto StartAcceptInBackground() -> void {
		this->io_service.post([this](){
			this->front_os << boost::format("ServerReady:\"%1%\"") 
				% this->node_id.ToString()
			<< std::endl;
			this->server.StartAccept();
		});
	}

	auto StartCuiShell() -> void {
		this->cui_shell.Call("setdd ./download");
		this->cui_shell.Start();	
	}

	auto Call(const std::string& command) -> void {
		this->cui_shell.Call(command);	
	}

private:
	std::ostream& front_os;
	std::ostream& log_os;
	std::istream& is;
	boost::asio::io_service& io_service;
	boost::asio::io_service::work work;
	const database::NodeId node_id;
	const neuria::network::BufferSize buffer_size;
	neuria::network::Server server;
	neuria::shell::CuiShell cui_shell;
	neuria::command::CommandDispatcher command_dispatcher;
	neuria::network::Client client;
	neuria::network::ConnectionPool connection_pool;//<-thread safe :)
	database::FileKeyHashDb::Ptr search_file_key_hash_db;//<- thread safe too ;)
	database::FileKeyHashDb::Ptr spread_file_key_hash_db;//<- thread safe too ;)
	neuria::thread_safe::ThreadSafeVariable<database::FileSystemPath> 
		download_directory_path;
	const command::HopCount max_hop_count;
	const unsigned int spread_max_count;
	neuria::timer::MultipleTimer multiple_timer;
	unsigned int spread_file_key_hash_interval;
	unsigned int check_upload_directory_interval;
};
}
