// Lab1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <vector>
#include <map>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <algorithm>
#include <time.h>
#include <stdio.h>

#define DO_SYNCHRONIZE

#define NUM_THREADS             200
#define NUM_PRODUCTS            20
#define MIN_PRICE               1
#define MAX_PRICE               19
#define MIN_QUANTITY            20
#define MAX_QUANTITY            100
#define MAX_PRODUCTS_SALE       20      // maximum products in a sale

using namespace std;

class Product {
private:
	int id;
	string name;
	int price;

public:
	Product(int id, string name, int price)
	{
		this->id = id;
		this->name = name;
		this->price = price;
	}

	Product(int id)
	{
		this->id = id;
		this->name = "";
		this->price = MIN_PRICE;
	}


	Product()
	{
		this->id = -1;
		this->name = "";
		this->price = MIN_PRICE;
	}


	int getId()
	{
		return this->id;
	}

	string getName()
	{
		return this->name;
	}

	int getPrice()
	{
		return this->price;
	}

	bool operator==(const Product& other)
	{
		return this->id == other.id;
	}

	bool operator!=(const Product& other)
	{
		return !(*this == other);
	}

	bool operator<(const Product& other) const
	{
		return this->id < other.id ? true : false;
	}

	friend ostream& operator<<(ostream& os, const Product& product)
	{
		os << "Product{id=" << product.id << ", name=" << product.name << ", price=" << product.price << "}";
		return os;
	}

	friend string to_string(Product& product)
	{
		return "Product{id=" + to_string(product.id) + ", name=" + product.name + ", price=" + to_string(product.price) + "}";
	}
};

class Inventory {
private:
	map<Product, int> inventory;
public:
	static mutex inventoryMutex;

	Inventory()
	{

	}

	Inventory(map<Product, int> inventory)
	{
		this->inventory = inventory;
	}

	Inventory& operator=(const Inventory& other)
	{
		if (this != &other)
		{
			this->inventory = other.inventory;
		}
		return *this;
	}

	int getProductQuantity(Product p)
	{
		return this->inventory[p];
	}

	Product getProduct(int productId)
	{
		for (map<Product, int>::iterator it = this->inventory.begin(); it != this->inventory.end(); it++)
		{
			Product p = it->first;
			if (p.getId() == productId)
			{
				return p;
			}
		}
		return Product();
	}

	bool addProduct(Product p, int quantity)
	{
		if (0 == this->inventory.count(p))
		{
			this->inventory[p] = quantity;
		}
		else
		{
			this->inventory[p] += quantity;
		}
		return true;
	}

	bool removeProduct(Product p, int quantity)
	{
		if (0 == this->inventory.count(p))
		{
			return false;
		}
		else if (quantity > this->inventory[p])
		{
			return false;
		}
		else
		{
			this->inventory[p] -= quantity;
			return true;
		}
	}

	friend ostream& operator<<(ostream& os, const Inventory& inventory);
	friend string to_string(const Inventory& inventory);
};

ostream& operator<<(ostream &os, const Inventory &inventory)
{
	os << "Inventory:" << endl;
	for (map<Product, int>::const_iterator it = inventory.inventory.begin(); it != inventory.inventory.end(); it++)
	{
		Product product = it->first;
		int quantity = it->second;
		os << "\t" << product << ", quantity: " << quantity << endl;
	}

	return os;
}

string to_string(const Inventory &Inventory)
{
	string str = "Inventory:\n";

	for (map<Product, int>::const_iterator it = Inventory.inventory.begin(); it != Inventory.inventory.end(); it++)
	{
		Product product = it->first;
		int quantity = it->second;
		str = str + "\t" + to_string(product) + ", quantity: " + to_string(quantity) + "\n";
	}
	return str;
}

class Bill {

private:
	map<Product, int> items;
	int totalPrice;

public:

	Bill()
	{
		this->items = map<Product, int>();
		this->totalPrice = 0;
	}

	int getTotalPrice()
	{
		return this->totalPrice;
	}

	map<Product, int> getItems()
	{
		return this->items;
	}

	void addItem(Product p, int quantity)
	{
		if (0 == this->items.count(p))
		{
			this->items[p] = quantity;
		}
		else
		{
			this->items[p] += quantity;
		}

		this->totalPrice += quantity * p.getPrice();
	}

	friend ostream& operator<<(ostream &os, const Bill &bill);
};

ostream& operator<<(ostream& os, const Bill& bill)
{
	os << "Bill:\n";
	int i = 1;
	for (map<Product, int>::const_iterator it = bill.items.begin(); it != bill.items.end(); it++, i++)
	{
		os << "\t" << i << ". " << it->first << ", quantity: " << it->second << endl;
	}
	os << "\t--------------------" << endl;
	os << "\tTOTAL: " << bill.totalPrice << endl;
	return os;
}

class Supermarket
{
private:
	Inventory inventory;
	Inventory soldItems;
	int money;
	vector<Bill> bills;

	static mutex inventoryAndSoldItemsMutex;
	static mutex billsAndMoneyMutex;

public:
	Supermarket()
	{
		this->inventory = Inventory();
		this->soldItems = Inventory();
		this->money = 0;
		this->bills = vector<Bill>();
	}

	Supermarket(Inventory inventory)
	{
		this->inventory = inventory;
		this->soldItems = Inventory();
		this->money = 0;
		this->bills = vector<Bill>();
	}

	void sale(map<Product, int> products)
	{
		Bill bill = Bill();

#ifdef DO_SYNCHRONIZE
		Supermarket::billsAndMoneyMutex.lock();
#endif // DO_SYNCHRONIZE

		for (map<Product, int>::iterator it = products.begin(); it != products.end(); it++)
		{
			Product product = it->first;
			product = this->inventory.getProduct(product.getId());
			int quantity = it->second;

			if (this->inventory.getProductQuantity(product) < quantity)
			{
				printf("Cannot perform sale because of item unavailability. %s. Quantity %d.\n", to_string(product).c_str(), quantity);
				continue;
			}

#ifdef DO_SYNCHRONIZE
			Supermarket::inventoryAndSoldItemsMutex.lock();
#endif // DO_SYNCHRONIZE
			this->inventory.removeProduct(product, quantity);
			this->soldItems.addProduct(product, quantity);
#ifdef DO_SYNCHRONIZE
			Supermarket::inventoryAndSoldItemsMutex.unlock();
#endif // DO_SYNCHRONIZE

			bill.addItem(product, quantity);
		}

		this->money += bill.getTotalPrice();
		this->bills.push_back(bill);
#ifdef DO_SYNCHRONIZE
		Supermarket::billsAndMoneyMutex.unlock();
#endif // DO_SYNCHRONIZE
	}

	bool checkInventory(int threadId)
	{
		bool status = true;

		// I. check money
		int computedMoney = 0;

#ifdef DO_SYNCHRONIZE
		Supermarket::billsAndMoneyMutex.lock();
#endif // DO_SYNCHRONIZE

		for (unsigned int i = 0; i < this->bills.size(); i++)
		{
			computedMoney += this->bills[i].getTotalPrice();
		}

		if (computedMoney != this->money)
		{
			printf("[T %d] sum(total(bills)) = %d, but total money = %d.\n", threadId, computedMoney, this->money);
			status = false;
		}

#ifdef DO_SYNCHRONIZE
		Supermarket::billsAndMoneyMutex.unlock();
#endif // DO_SYNCHRONIZE

		// II. check sold products
		map<Product, int> soldItemsCheck = map<Product, int>();
#ifdef DO_SYNCHRONIZE
		Supermarket::billsAndMoneyMutex.lock();
		Supermarket::inventoryAndSoldItemsMutex.lock();
#endif // DO_SYNCHRONIZE
		for (unsigned int i = 0; i < this->bills.size(); i++)
		{
			map<Product, int> billItems = this->bills[i].getItems();

			for (map<Product, int>::iterator bIt = billItems.begin(); bIt != billItems.end(); bIt++)
			{
				Product product = (*bIt).first;
				int quantity = (*bIt).second;

				if (soldItemsCheck.count(product) == 0)
				{
					soldItemsCheck[product] = quantity;
				}
				else
				{
					soldItemsCheck[product] += quantity;
				}
			}
		}

		for (map<Product, int>::iterator it = soldItemsCheck.begin(); it != soldItemsCheck.end(); it++)
		{
			Product product = (*it).first;
			int quantity = (*it).second;

			if (quantity != this->soldItems.getProductQuantity(product))
			{
				string str = "[T " + to_string(threadId) + "] ";
				str = str + "Product: " + to_string(product) + ":\n";
				str = str + "\tvanzari pe factura: " + to_string(this->soldItems.getProductQuantity(product));
				str = str + ", vanzari reale: " + to_string(quantity);

				printf("%s\n", str.c_str());

				status = false;
			}
		}

#ifdef DO_SYNCHRONIZE
		Supermarket::inventoryAndSoldItemsMutex.unlock();
		Supermarket::billsAndMoneyMutex.unlock();
#endif // DO_SYNCHRONIZE

		if (status == true)
		{
			printf("[T %d] Check operation success. Everything is ok.\n", threadId);
		}
		else {
			printf("[T %d] Check operation FAILED. Inconsistencies were found\n", threadId);
		}

		return status;
	}

	friend ostream& operator<<(ostream& os, const Supermarket& supermarket);
};

ostream& operator<<(ostream& os, const Supermarket& supermarket)
{
	os << "Supermarket:\n";
	os << "Supermarket's inventory: " << supermarket.inventory << endl;
	os << "Supermarket's amount of money: " << supermarket.money << endl;
	os << "Supermarket's bills:\n";
	for (vector<Bill>::const_iterator it = supermarket.bills.begin(); it != supermarket.bills.end(); it++)
	{
		os << "\tBill: " << (*it) << endl;
	}
	return os;
}

Supermarket gSupermarket;
mutex Inventory::inventoryMutex;
mutex Supermarket::inventoryAndSoldItemsMutex;
mutex Supermarket::billsAndMoneyMutex;

void threadFunction(int threadId)
{
	int operation;

	printf("[T %d] started.\n", threadId);
	srand((time(NULL) - (threadId * 14 - 29)));

	operation = rand() % 2;

	if (0 == operation)
	{
		int numProductsToBuy;
		printf("[Thread %d] is performing a sale operation.\n", threadId);

		map<Product, int> products = map<Product, int>();
		numProductsToBuy = 1 + rand() % MAX_PRODUCTS_SALE;

		for (int i = 0; i < numProductsToBuy; i++)
		{
			int pId = rand() % NUM_PRODUCTS;
			Product p = Product(pId);

			if (0 == products.count(p))
			{
				products[p] = 1;
			}
			else
			{
				products[p] += 1;
			}
		}
		gSupermarket.sale(products);
	}
	else
	{
		printf("[Thread %d] is performing a check operation.\n", threadId);

		gSupermarket.checkInventory(threadId);
	}

	printf("[Thread %d] finished.\n", threadId);
}

void initSupermarketInventory(int numProducts, int minQuantity, int maxQuantity, int minPrice, int maxPrice)
{
	Inventory inventory = Inventory();

	for (int id = 0; id < numProducts; id++)
	{
		string name = "prod#" + to_string(id);
		int price = minPrice + (rand() % static_cast<int>(maxPrice - minPrice + 1));
		int quantity = minQuantity + (rand() % static_cast<int>(maxQuantity - minQuantity + 1));

		inventory.addProduct(Product(id, name, price), quantity);
	}

	gSupermarket = Supermarket(inventory);
}

int main()
{
	vector<thread> threads = vector<thread>();

	srand(static_cast<int>(time(NULL)));

	initSupermarketInventory(NUM_PRODUCTS, MIN_QUANTITY, MAX_QUANTITY, MIN_PRICE, MAX_PRICE);
	cout << gSupermarket << endl;

	for (int tId = 1; tId <= NUM_THREADS; tId++)
	{
		threads.push_back(thread(&threadFunction, tId));
	}

	for (int tId = 0; tId < NUM_THREADS; tId++)
	{
		threads[tId].join();
	}

	gSupermarket.checkInventory(0);

	cout << gSupermarket << endl;

	return 0;
}
