{
        'targets':
        [
                {
                'target_name': 'recorder',
                'type': 'executable',
                'include_dirs':[],
                'sources': [
                        'main.cpp',
                        'NetworkManager.cpp',
                        'NetworkListener.cpp',
                        'NetworkListenerImpl.cpp',
                        'message/Message.cpp',
                        'message/InputSerialize.cpp',
			'web_socket/WebSocket.cpp',
                ],
                'conditions':[
                        ['OS=="win"',{
                                'cflags':[
					'-g3',
					],
                                'ldflags':[
					],
				'libraries':[
                                  '-lwebsockets',
                              		]
                                },{
                                'cflags':[
                                        '--std=c++11',
					'-g3',
                                        ],
                                'ldflags':[
					],
				'libraries':[
                                  '-lwebsockets',
                              		]
                                }
                        ],
                ]
                },
        ],
}

