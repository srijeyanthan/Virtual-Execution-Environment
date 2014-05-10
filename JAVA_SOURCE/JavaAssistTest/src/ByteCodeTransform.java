import java.util.ArrayList;
import javassist.ClassPool;
import javassist.CtClass;
import javassist.CtField;
import javassist.CtMethod;

public class ByteCodeTransform {
	
	private static String className;
	private static String destinationPath;
	private static String classPath;
	private static final String INVALID_METHOD_NAMES="wait,equals,main,clone,getClass,finalize,notifyAll,hashCode,notify,toString";
    private static final String PREFIX_GLOBAL_VARIABLE_START="group2S_";
    private static final String PREFIX_GLOBAL_VARIABLE_END="group2E_";
    private static final String PREFIX_GETTER_METHOD="";
    private static final String METHOD_NAME_MAIN="main";
    private static final String CLASS_VARIABLE_NAME="var";
	public static void main(String[] args)  {
    	boolean isManualOperation=false;
    	classPath="E:\\EMDC_IST\\SEMESTER_2\\AVExe_Virtual_Execution_Environments\\PROJECT\\NewWorkspace\\JavaAssistTest\\newcls";
    	className="Hello";
    	destinationPath="E:\\EMDC_IST\\SEMESTER_2\\AVExe_Virtual_Execution_Environments\\PROJECT\\NewWorkspace\\JavaAssistTest\\dest";
    	if(args!=null && args.length==3 && !isManualOperation)
    	{
	    	classPath=args[0];
	    	className=args[1];
	    	destinationPath=args[2];
	        StartTransform();
    	}
    	else if(isManualOperation)
    	{
	        StartTransform();
    	}
    	else
    	{
    		System.err.println("Please pass argumets\n[0]-classPath\n[1]-className\n[2]-destinationPath\n[3]-thresholdValue");
    	}
    }
    
    public static void StartTransform()
    {
    	try
    	{
	    	ClassPool classPool = ClassPool.getDefault();
	    	classPool.insertClassPath(classPath);
	        CtClass clasRef = classPool.get(className);
	        CtMethod[] validMethodArray= GetMethods(clasRef);
	        CtMethod mainMethod=clasRef.getDeclaredMethod(METHOD_NAME_MAIN);
	        CtField field=null;
	        if(validMethodArray!=null)
	        {
	        	String fieldName;
	        	String methodName;
	        	
	        	field=CtField.make("public static "+ clasRef.getName() +" "+CLASS_VARIABLE_NAME+"=new "+clasRef.getName()+"();", clasRef);
	        	clasRef.addField(field);
	        	
	        	for(CtMethod method:validMethodArray)
	        	{
        				fieldName=PREFIX_GLOBAL_VARIABLE_END+clasRef.getName()+method.getName();		        		
	        			methodName=PREFIX_GETTER_METHOD+fieldName;
		        		field=CtField.make("private static int "+fieldName+"=0;", clasRef);
		        		clasRef.addField(field);		                
	        			method.insertAfter("{ "+fieldName+"++;}");		        		
		                CtMethod getterMethod=CtMethod.make(GenerateGetter(method, methodName,fieldName, clasRef), clasRef);
		                clasRef.addMethod(getterMethod);
		                mainMethod.insertBefore("{"+CLASS_VARIABLE_NAME+"."+methodName+"();}");
	        	} 
	        	
	        	for(CtMethod method:validMethodArray)
	        	{
	        		    fieldName=PREFIX_GLOBAL_VARIABLE_START+clasRef.getName()+method.getName();
	        			methodName=PREFIX_GETTER_METHOD+fieldName;
		        		field=CtField.make("private static int "+fieldName+"=0;", clasRef);
		        		clasRef.addField(field);
	        			method.insertBefore("{ "+fieldName+"++;}");	        		
		                CtMethod getterMethod=CtMethod.make(GenerateGetter(method, methodName,fieldName, clasRef), clasRef);
		                clasRef.addMethod(getterMethod);
		                mainMethod.insertBefore("{"+CLASS_VARIABLE_NAME+"."+methodName+"();}");            	                
	        	}
             	                
	        	
	        	/*
	        	for(CtMethod method:validMethodArray)
	        	{
	        		for(int i=1;i<=2;i++)
	        		{
	        			if(i==1)
	        				fieldName=PREFIX_GLOBAL_VARIABLE_START+clasRef.getName()+method.getName();
	        			else
	        				fieldName=PREFIX_GLOBAL_VARIABLE_END+clasRef.getName()+method.getName();
		        		
	        			methodName=PREFIX_GETTER_METHOD+fieldName;
		        		field=CtField.make("private static int "+fieldName+"=0;", clasRef);
		        		clasRef.addField(field);
		                
		        		if(i==1)
		        			method.insertBefore("{ "+fieldName+"++;}");
		        		else
		        			method.insertAfter("{ "+fieldName+"++;}");
		        		
		                CtMethod getterMethod=CtMethod.make(GenerateGetter(method, methodName,fieldName, clasRef), clasRef);
		                clasRef.addMethod(getterMethod);
		                mainMethod.insertBefore("{"+CLASS_VARIABLE_NAME+"."+methodName+"();}");
	        		}              	                
	        	}*/
	        	
        		//CtMethod newOverloadedMethod=CtMethod.make(GenerateHookMethod(validMethodArray, clasRef), clasRef);
        		//clasRef.addMethod(newOverloadedMethod);	        	
	        	clasRef.writeFile(destinationPath);	        	
	        } 
    	}
    	catch (Exception e) 
    	{
			System.err.println(e.getMessage());
		}
    }
    
    public static String GenerateGetter(CtMethod method,String methodName,String fieldName,CtClass clasRef)
    {
    	StringBuilder sb=new StringBuilder();
    	sb.append("public  int "+methodName+"(){");
    	//sb.append("System.out.println(\""+fieldName+"\");");
    	sb.append("return "+fieldName+";");
    	sb.append("}");
    	return sb.toString();
    }
   /* 
    public static String GenerateHookMethod(CtMethod[] validMethodArray,CtClass clasRef)
    {
    	StringBuilder sb=new StringBuilder();
    	sb.append("public void HookMethod(){");
    	String fieldName;
    	for(CtMethod method:validMethodArray)
    	{
    		fieldName=method.getName()+PREFIX_GLOBAL_VARIABLE;
    		sb.append("if("+fieldName+"==100)");
    		sb.append("System.out.println(\"Call Dummy_"+clasRef.getName()+"_"+method.getName()+"_one() \");");
    		sb.append("else if("+fieldName+"==200)");
    		sb.append("System.out.println(\"Call Dummy_"+clasRef.getName()+"_"+method.getName()+"_two() \");");
    		sb.append("else if("+fieldName+"==300)");
    		sb.append("System.out.println(\"Call Dummy_"+clasRef.getName()+"_"+method.getName()+"_three() \");");
            	                
    	}
    	sb.append("}");
    	return sb.toString();
    }
    */
    public static boolean IsValidateMethod(String methodName)
    {    	
    	boolean isValid=true;
    	String[] invalidMethodNameList=INVALID_METHOD_NAMES.split(",");
    	for(int i=0;i<invalidMethodNameList.length;i++)
    	{
    		if(methodName.equals(invalidMethodNameList[i]))
    		{
    			isValid=false;
    			break;
    		}
    	}
    	return isValid;
    }
    
    public static CtMethod[] GetMethods(CtClass classRef)
    {
    	ArrayList<CtMethod> methodsInterested =new ArrayList<CtMethod>();
    	CtMethod[] list=classRef.getMethods();
    	if(list!=null && list.length>0)
    	{
    		for(int i=0;i<list.length;i++)
    		{
    			if(IsValidateMethod(list[i].getName()))
    				methodsInterested.add(list[i]);
    		}    		
    	}
    	return (methodsInterested.size()>0)?methodsInterested.toArray(new CtMethod[methodsInterested.size()]):null;
    }
}
