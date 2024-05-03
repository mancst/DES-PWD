## Project Description
The framework commences by employing workload prediction to evaluate host status for determining 
the most opportune timing for migration. Subsequently, we leverage Service Level Agreements (SLA) 
violation as the optimization objective to ascertain the optimal status threshold, thereby 
facilitating effective workload partition of the host. The framework employs multi-dimensional 
host resource balance as a guide to schedule host migration in diverse areas, ensuring robust resource 
utilization post-migration. 


**One-click deployment (source code compilation)**  
Installation script: local_install.sh  
Uninstall script: uninstall_DES-PWE.sh   

Console default account/password: sadmin/sadmin  


**Technical solutions used:**  
1. apache + mysql(Monitoring point data and configuration information are stored using mysql, supporting distributed deployment.)   
2. Front-end web console uses [dwz](http://jui.org/)   
3. Front-end charts are open source [echarts](https://www.echartsjs.com/zh/index.html)
4. Backend cgi uses the open source cgi template engine - [clearsilver](http://www.clearsilver.net/), All cgi supports deployment in fastcgi mode
5. The background service uses the open source socket development framework - [C++ Sockets](http://www.alhem.net/Sockets/)   

**Advantages compared to other migration cloud management systems:**    
1.	Hybrid evaluation strategy provides a short-term prediction for the host's workload. 
2.	Performance of the strategy declines during the workload fluctuations.    
3.  Error fitting keeps performance of the strategy up during workload fluctuations.
4.  Partitioned workload evaluation fosters best practices for workload optimization.
5.  The framework reduces SLA violations by 27% with reduced energy consumption.
   
	  



## Contact us

Mail：luolj@hbut.edu.cn

