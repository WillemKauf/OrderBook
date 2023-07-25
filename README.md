# Limit Order Book

This is a program which accepts orders and cancellations over UDP port `8888`, maintaining multiple price-time order books (one book per symbol), and publishes acknowledgements, trades, and top of book (TOB) changes using multiple threads.

# Input/Output Data

This is redacted for confidentality reasons. Input files should be of the form

```
#Scenario
//Order 1
//Order 2
//...
```

Using the form:

Market/Limit buy/sells:

`N, user (int), symbol (string), price (int), quantity (int), side (string, B or S), orderId (int)`

Cancel orders:

`C, user (int), orderId (int)`

Flush (End round of orders, write output to standard out and `/logs/` directory):

`F`

i.e

```
#Scenario
N, 1, TSLA, 15, 150, B, 1
N, 2, TSLA, 20, 101, S, 2
C, 1, 1
F
```

Output reflects acknowledgement of orders, as well as updates regarding the TOB on either side.

# Dependencies

* [pybind11](https://github.com/pybind/pybind11) (For reading input data. Much easier to parse in Python.)
* [Cereal](https://uscilab.github.io/cereal/) (For serialization of order data to be sent across UDP sockets. Protobuf was also an option, but I opted for the ease of a library like `Cereal`).
* [gtest](https://github.com/google/googletest) (For testing, of course.)
* C++20 friendly compiler.

### Order Books

Server is ran in one process, clients can send buy/sell/cancel requests from any other process on a given UDP port.

My `Orderbooks` implementation contains an `std::unordered_map<Symbol_type, OrderBook>` for mapping from a stock ticker to an order book. There are several key features of the `OrderBook`. Red-black binary search trees are used to sort and store orders on both buy and sell sides at different limits. A `Limit` is comprised of a doubly linked list and a `int totalQuantity` at that limit price. The doubly linked list stores all orders at a given limit. Furthermore, a hash map is used for limit lookup after the level has first been added to the binary search tree, allowing for constant average time lookup instead of logarithmic. A hash map in the `Orderbooks` is also used to index all orders, once again allowing for constant time lookup for a cancellation.

Finally, we keep iterators to the top of book on both sides, which provide constant time lookup for orders (since the BSTs are sorted). They must be kept updated as the binary search trees are modified, however (orders filled, added, cancelled, etc).
The top of book (TOB) refers to the highest offer and the lowest bid, as those will be the first limit orders filled. Market orders are also easily supported in this design.
[This is the blog post most commonly referenced when it comes to this design.](https://web.archive.org/web/20110219163448/http://howtohft.wordpress.com/2011/02/15/how-to-build-a-fast-limit-order-book/)
But I first read about it in [Ace the Trading Systems Developer Interview by Dennis Thompson, a highly recommended resource.](https://books.google.ca/books?id=68P1DwAAQBAJ&printsec=copyright&redir_esc=y#v=onepage&q&f=false)

The following operations are supported:

* Adding a new Order (Buy/Sell): `O(log n)` for first order at a limit price, `O(1)` -> `O(n)` for all others.
  * The first time a limit is added to the binary search tree, `O(log n)` lookup must be performed (`[]` operator, or `map::insert_or_assign()`). However, after a limit is stored in the binary search tree, a hash map can be used to index the limit instead, bringing the time complexity to `O(1)` on average (and `O(n)` in the worst case, due to collisions).
* Get top of book/perform trades (Buy/Sell): `O(1)`.
  * We can use our TOB iterators to index through available buy/sells, and execute the trade if we are able.
* Cancel an order: `O(1) -> O(n)`
* We get a pointer from a hash map given `userOrderId` as a key to the doubly linked list that contains the order, and delete it from the doubly linked list. This is once again constant time, bar any collisions in the hash map.
