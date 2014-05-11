import java.util.Random;

public class Game
{

    private static int group2S_GameCalc=0;
    private static int group2S_GameCalc2=0;
    private static int group2S_GameCalc3=0;
     private static int group2E_GameCalc=0;
    private static int group2E_GameCalc2=0;
    private static int group2E_GameCalc3=0;
    int factorial(int n) {
        int fact = 1; // this  will be the result
        for (int i = 1; i <= n; i++) {
            fact *= i;
        }
        return fact;
    }

    public void Calc()
    {
      ++group2S_GameCalc;
          for(int i =1; i <500 ; ++i)
               factorial(i);  
      ++group2E_GameCalc;
      
    }
    public void Calc2()
    {
      ++group2S_GameCalc2;
           for(int i =1; i <500 ; ++i)
               factorial(i);
      ++group2E_GameCalc2;

    }
public void Calc3()
    {
      ++group2S_GameCalc3;
           for(int i =1; i <1000 ; ++i)
               factorial(i);

      ++group2E_GameCalc3;
    }

   
  int  group2S_GameCalc()
    {
        return group2S_GameCalc;
    }

  int group2S_GameCalc2()
    {
        return group2S_GameCalc2;
    }
  int group2S_GameCalc3()
   {
         return group2S_GameCalc3;
   }
  int  group2E_GameCalc()
    {
        return group2E_GameCalc;
    }

   int group2E_GameCalc2()
    {
        return group2E_GameCalc2;
    }
  int group2E_GameCalc3()
   {
         return group2E_GameCalc3;
   }

    public static void main(String[] args)
   {

      Game gg = new Game();
      Random rr = new Random();
      gg.group2S_GameCalc();
      gg.group2S_GameCalc2();
      gg.group2S_GameCalc3();
      gg.group2E_GameCalc();
      gg.group2E_GameCalc2();
      gg.group2E_GameCalc3();
    for(int j =0; j <100 ; ++j)
     {
      gg.Calc();
      gg.Calc2();
     }

    System.out.println("****************************************************");
  for(int j =0; j <5000 ; ++j)
     {
      gg.Calc3(); 
         try 
            {
            int sleeptime =  rr.nextInt(100);
            Thread.sleep((long)(sleeptime));
            }
            catch (Exception e)
           {

           }
     }
  for(int j =0; j <10 ; ++j)
     {
      gg.Calc();
   
     }

  for(int j =0; j <10 ; ++j)
     {
      gg.Calc2();
     }

   }

}
