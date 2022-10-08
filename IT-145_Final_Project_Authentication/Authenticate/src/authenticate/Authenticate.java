package authenticate;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.Scanner;
import java.security.MessageDigest;

public class Authenticate {
    // Declare variables
    public static User temporaryUser = new User(); 
    public static File adminFile = new File("admin.txt");
    public static File veterinarianFile = new File("veterinarian.txt");
    public static File zookeeperFile = new File("zookeeper.txt");
    public static File guideFile = new File("guide.txt");
    public static int attemptCounter = 0;
    public static boolean successfulLogin = false;
    public static Scanner userInput = new Scanner(System.in);
    public static boolean logout = false;
    
    // Login loop
    public static void main(String []args) throws Exception{
        while (!logout) {
            startLogin();
            if (successfulLogin) {
                promptLogout();
            }
        }
    }
    
    // Login process
    public static void startLogin() throws Exception {
        User[] allUsers = createUsers();
        attemptCounter = 0;
        successfulLogin = false;
        
        // Login attempt counter
        while (attemptCounter < 3  && !successfulLogin) {
            getTemporaryUserCredentials(userInput);
            for (User u : allUsers) {
                if (temporaryUser.username.equals(u.username)) {
                    if (temporaryUser.encryptedPassword.equals(u.encryptedPassword)) {
                        printFile(u.role);
                        successfulLogin = true;
                        break;
                    }
                }
            }
            attemptCounter++;
        }
        
        // Message after three failed login attempts
        if (attemptCounter == 3 && !successfulLogin) {
            userInput.close();
            logout = true; 
            System.out.println("\nIncorrect log in attempt limit has been reached.\n \nExiting program.");
        }
    }
    
    // Logout prompt
    public static void promptLogout() {
        System.out.println("\nLog out? \nType \"y\" for YES or \"n\" for NO.");
        if ("y".equals(userInput.next())) {
            logout = true;
        }
        userInput.nextLine();
    }
    
    // Username and password prompt
    public static void getTemporaryUserCredentials(Scanner userInput) throws Exception {
        System.out.print("Enter username: ");
        temporaryUser.username = userInput.nextLine();
        System.out.print("Enter password: ");
        temporaryUser.encryptedPassword = encrypt(userInput.nextLine());
    }
    
    // Hash function
    public static String encrypt(String original) throws Exception {
        StringBuffer sb = new StringBuffer();
        MessageDigest md = MessageDigest.getInstance("MD5");
        md.update(original.getBytes());
        byte[] digest = md.digest();
        for (byte b : digest) {
            sb.append(String.format("%02x", b & 0xff));
        }
        return sb.toString();
	}
    
    // Create users
    public static User[] createUsers() throws Exception {
        User users[] = new User[6];
        int indexCounter = 0;
        File credentialsFile = new File("credentials.txt");
        String pattern = "[^\"\\s]+|\"(\\\\.|[^\\\\\"])*\"";
        Scanner fileReader = new Scanner(credentialsFile);

        // fileReader users loop
        while (fileReader.hasNextLine()) {
            users[indexCounter] = new User();
            users[indexCounter].username = fileReader.findInLine(pattern);
            users[indexCounter].encryptedPassword = fileReader.findInLine(pattern);
            users[indexCounter].password = fileReader.findInLine(pattern);
            users[indexCounter].role = fileReader.findInLine(pattern);
            if (fileReader.hasNextLine() == true) {
                fileReader.nextLine();
            }
            indexCounter++;
        }
        fileReader.close();
        return users;
    }
     // Print contents of respective role file
    public static void printFile(String role) throws Exception {
        System.out.println();
        switch (role) {
            case "admin":
                Scanner fileReader = new Scanner(adminFile);
                while (fileReader.hasNextLine()) {
                    System.out.println(fileReader.nextLine());
                }
                break;
            case "veterinarian":
                Scanner fileReader2 = new Scanner(veterinarianFile);
                while (fileReader2.hasNextLine()) {
                    System.out.println(fileReader2.nextLine());
                }
                break;
            case "zookeeper":
                Scanner fileReader3 = new Scanner(zookeeperFile);
                while (fileReader3.hasNextLine()) {
                    System.out.println(fileReader3.nextLine());
                }
                break;
            case "guide":
                Scanner fileReader4 = new Scanner(guideFile);
                while (fileReader4.hasNextLine()) {
                    System.out.println(fileReader4.nextLine());
                }
                break;
            default:
                SSystem.out.println("Invalid role");
        }
    }
}

// User class
class User {
    String username;
    String password;
    String encryptedPassword;
    String role;
}