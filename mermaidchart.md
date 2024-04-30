## 流程图

graph TD
    A[开始调试会话] --> B[初始化调试器]
    B --> C[加载程序并设置初始断点]
    C --> E[运行程序]
    E --> F[等待信号]
    F -->|SIGTRAP断点触发| I{决定下一步行动}
    F -->|其他信号| Y[输出信息]

    I -->|修改寄存器| J[应用修改]
    I -->|单步执行| K[单步过]
    I -->|继续执行| L[继续执行]
    I -->|添加/移除断点| M[更新断点]
    I -->|退出调试| N[结束调试会话]
    J --> E
    K --> E
    L --> E
    M --> E
    N --> O[解除附加并清理]


    style A fill:#f9f,stroke:#333,stroke-width:4px
    style N fill:#f9f,stroke:#333,stroke-width:4px
    style O fill:#f9f,stroke:#333,stroke-width:4px




## 系统架构
flowchart TD
    A[调试器核心 Debugger] --> B[断点管理 Breakpoint]
    A --> C[寄存器管理 Registers]
    A --> D[用户界面UI]
    
    D --> E[图形用户界面GUI]
    D --> F[命令行用户界面CLI]
    
    A --> G[辅助工具模块]
    
    G --> H[Utility类模块]
    G --> I[SymbolType类模块]
    G --> J[ASMParser类模块]




## 架构图
flowchart TB
    subgraph 核心模块
        Debugger
    end

    subgraph 辅助模块
        Utility
        Asmparaser
        Symboltype
        Ptrace_expr_context
        Breakpoint
        Registers
    end

    subgraph 用户界面
        图形界面
        用户交互
    end

    Debugger --> Breakpoint
    Debugger --> Registers
    Debugger --> Ptrace_expr_context
    Debugger --> Utility
    Debugger --> Asmparaser
    Debugger --> Symboltype

    图形界面 --> Debugger
    用户交互 --> Debugger



## asm_
graph TD
    A[Start: get_asm_data] --> B[Open file]
    B --> C{File opened?}
    C -->|No| D[Log error and return empty vector]
    C -->|Yes| E[Read file line by line]

    E --> F{Check line}
    F -->|Empty| G[Skip]
    F -->|Disassembly header| H[Skip]
    F -->|Contains tab: Instruction | I[Process asm_entry]
    F -->|No tab: Function header| J[Process asm_head]

    I --> K[Add entry to last asm_head]
    J --> L[Create new asm_head and add to result]

    L --> M[Set end_addr for each asm_head]
    K --> M
    M --> N[Close file and return result]


    classDef default stroke:#333, stroke-width:1px;
    class A,B,C,D,E,F,G,H,I,J,K,L,M,N default;
